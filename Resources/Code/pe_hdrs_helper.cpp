#include "pe_hdrs_helper.h"
#include "pe_helper.h"
#include <iostream>
#include <windows.h>
#include "lib/lazy_importer.hpp"


BYTE* get_nt_hdrs(IN const BYTE *pe_buffer, IN OPTIONAL size_t buffer_size)
{
    if (!pe_buffer) return nullptr;

    IMAGE_DOS_HEADER *idh = (IMAGE_DOS_HEADER*)pe_buffer;
    if (buffer_size != 0) {
        if (!validate_ptr((LPVOID)pe_buffer, buffer_size, (LPVOID)idh, sizeof(IMAGE_DOS_HEADER))) {
            return nullptr;
        }
    }
    else {
        if (is_bad_read_ptr(idh, sizeof(IMAGE_DOS_HEADER))) {
            return nullptr;
        }
    }
    if (idh->e_magic != IMAGE_DOS_SIGNATURE) {
        return nullptr;
    }
    const LONG kMaxOffset = 1024;
    LONG pe_offset = idh->e_lfanew;

    if (pe_offset > kMaxOffset) return nullptr;

    IMAGE_NT_HEADERS32 *inh = (IMAGE_NT_HEADERS32 *)(pe_buffer + pe_offset);
    if (buffer_size != 0) {
        if (!validate_ptr((LPVOID)pe_buffer, buffer_size, (LPVOID)inh, sizeof(IMAGE_NT_HEADERS32))) {
            return nullptr;
        }
    }
    else {
        if (is_bad_read_ptr(inh, sizeof(IMAGE_NT_HEADERS32))) {
            return nullptr;
        }
    }

    if (inh->Signature != IMAGE_NT_SIGNATURE) {
        return nullptr;
    }
    return (BYTE*)inh;
}

WORD get_nt_hdr_architecture(IN const BYTE* pe_buffer)
{
    void* ptr = get_nt_hdrs(pe_buffer);
    if (!ptr) return 0;

    IMAGE_NT_HEADERS32* inh = static_cast<IMAGE_NT_HEADERS32*>(ptr);
    if (is_bad_read_ptr(inh, sizeof(IMAGE_NT_HEADERS32))) {
        return 0;
    }
    return inh->OptionalHeader.Magic;
}

bool is_mem_accessible(LPCVOID areaStart, SIZE_T areaSize, DWORD dwAccessRights)
{
    if (!areaSize) return false; // zero-sized areas are not allowed

    const DWORD dwForbiddenArea = PAGE_GUARD | PAGE_NOACCESS;

    MEMORY_BASIC_INFORMATION mbi = { 0 };
    const size_t mbiSize = sizeof(MEMORY_BASIC_INFORMATION);

    SIZE_T sizeToCheck = areaSize;
    LPCVOID areaPtr = areaStart;

    while (sizeToCheck > 0) {
        //reset area
        memset(&mbi, 0, mbiSize);

        // query the next area
        if (LI_FN(VirtualQuery)(areaPtr, &mbi, mbiSize) != mbiSize) {
            return false; // could not query the area, assume it is bad
        }
        // check the privileges
        bool isOk = (mbi.State & MEM_COMMIT) // memory allocated and
            && !(mbi.Protect & dwForbiddenArea) // access to page allowed and
            && (mbi.Protect & dwAccessRights); // the required rights
        if (!isOk) {
            return false; //invalid access
        }
        SIZE_T offset = (ULONG_PTR)areaPtr - (ULONG_PTR)mbi.BaseAddress;
        SIZE_T queriedSize = mbi.RegionSize - offset;
        if (queriedSize >= sizeToCheck) {
            return true; // it is fine
        }
        // move to the next region
        sizeToCheck -= queriedSize;
        areaPtr = LPCVOID((ULONG_PTR)areaPtr + queriedSize);
    }
    // by default assume it is inaccessible
    return false;
}


bool is_bad_read_ptr(LPCVOID areaStart, SIZE_T areaSize)
{
#ifdef USE_OLD_BADPTR // classic IsBadReadPtr is much faster than the version using VirtualQuery
    return (IsBadReadPtr(areaStart, areaSize)) ? true : false;
#else
    const DWORD dwReadRights = PAGE_READONLY | PAGE_READWRITE | PAGE_WRITECOPY | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY;
    bool isAccessible = is_mem_accessible(areaStart, areaSize, dwReadRights);
    if (isAccessible) {
        // the area has read access rights: not a bad read pointer
        return false;
    }
    return true;
#endif
}


IMAGE_DATA_DIRECTORY* get_directory_entry(IN const BYTE *pe_buffer, IN DWORD dir_id, IN bool allow_empty)
{
    if (dir_id >= IMAGE_NUMBEROF_DIRECTORY_ENTRIES) return nullptr;

    BYTE* nt_headers = get_nt_hdrs((BYTE*)pe_buffer);
    if (!nt_headers) return nullptr;

    IMAGE_DATA_DIRECTORY* peDir = nullptr;
    if (is64bit(pe_buffer)) {
        IMAGE_NT_HEADERS64* nt_headers64 = (IMAGE_NT_HEADERS64*)nt_headers;
        peDir = &(nt_headers64->OptionalHeader.DataDirectory[dir_id]);
    }
    else {
        IMAGE_NT_HEADERS32* nt_headers64 = (IMAGE_NT_HEADERS32*)nt_headers;
        peDir = &(nt_headers64->OptionalHeader.DataDirectory[dir_id]);
    }
    if (!allow_empty && !peDir->VirtualAddress) {
        return nullptr;
    }
    return peDir;
}

bool update_image_base(IN OUT BYTE* payload, IN ULONGLONG destImageBase)
{
    bool is64b = is64bit(payload);
    BYTE* payload_nt_hdr = get_nt_hdrs(payload);
    if (!payload_nt_hdr) {
        return false;
    }
    if (is64b) {
        IMAGE_NT_HEADERS64* payload_nt_hdr64 = (IMAGE_NT_HEADERS64*)payload_nt_hdr;
        payload_nt_hdr64->OptionalHeader.ImageBase = (ULONGLONG)destImageBase;
    }
    else {
        IMAGE_NT_HEADERS32* payload_nt_hdr32 = (IMAGE_NT_HEADERS32*)payload_nt_hdr;
        payload_nt_hdr32->OptionalHeader.ImageBase = (DWORD)destImageBase;
    }
    return true;
}


WORD get_subsystem(IN const BYTE* payload)
{
    if (!payload) return 0;

    BYTE* payload_nt_hdr = get_nt_hdrs(payload);
    if (!payload_nt_hdr) {
        return 0;
    }
    const bool is64b = is64bit(payload);
    if (is64b) {
        IMAGE_NT_HEADERS64* payload_nt_hdr64 = (IMAGE_NT_HEADERS64*)payload_nt_hdr;
        return payload_nt_hdr64->OptionalHeader.Subsystem;
    } else {
        IMAGE_NT_HEADERS32* payload_nt_hdr32 = (IMAGE_NT_HEADERS32*)payload_nt_hdr;
        return payload_nt_hdr32->OptionalHeader.Subsystem;
    }
}


size_t get_sections_count(IN const BYTE* payload, IN const size_t buffer_size)
{
    const IMAGE_FILE_HEADER* fileHdr = get_file_hdr(payload, buffer_size);
    if (!fileHdr) {
        return 0;
    }
    return fileHdr->NumberOfSections;
}


const IMAGE_FILE_HEADER* get_file_hdr(IN const BYTE* payload, IN const size_t buffer_size)
{
    if (!payload) return nullptr;

    BYTE* payload_nt_hdr = get_nt_hdrs(payload, buffer_size);
    if (!payload_nt_hdr) {
        return nullptr;
    }
    if (is64bit(payload)) {
        return fetch_file_hdr(payload, buffer_size, (IMAGE_NT_HEADERS64*)payload_nt_hdr);
    }
    return fetch_file_hdr(payload, buffer_size, (IMAGE_NT_HEADERS32*)payload_nt_hdr);
}



template <typename IMAGE_NT_HEADERS_T>
inline const IMAGE_FILE_HEADER* fetch_file_hdr(IN const BYTE* payload, IN const size_t buffer_size, IN const IMAGE_NT_HEADERS_T *payload_nt_hdr)
{
    if (!payload || !payload_nt_hdr) return nullptr;

    const IMAGE_FILE_HEADER *fileHdr = &(payload_nt_hdr->FileHeader);

    if (!validate_ptr((const LPVOID)payload, buffer_size, (const LPVOID)fileHdr, sizeof(IMAGE_FILE_HEADER))) {
        return nullptr;
    }
    return fileHdr;
}




ULONGLONG get_image_base(IN const BYTE *pe_buffer)
{
    bool is64b = is64bit(pe_buffer);
    //update image base in the written content:
    BYTE* payload_nt_hdr = get_nt_hdrs(pe_buffer);
    if (!payload_nt_hdr) {
        return 0;
    }
    ULONGLONG img_base = 0;
    if (is64b) {
        IMAGE_NT_HEADERS64* payload_nt_hdr64 = (IMAGE_NT_HEADERS64*)payload_nt_hdr;
        img_base = payload_nt_hdr64->OptionalHeader.ImageBase;
    } else {
        IMAGE_NT_HEADERS32* payload_nt_hdr32 = (IMAGE_NT_HEADERS32*)payload_nt_hdr;
        img_base = static_cast<ULONGLONG>(payload_nt_hdr32->OptionalHeader.ImageBase);
    }
    return img_base;
}


DWORD get_image_size(IN const BYTE *payload)
{
    if (!get_nt_hdrs(payload)) {
        return 0;
    }
    DWORD image_size = 0;
    if (is64bit(payload)) {
        IMAGE_NT_HEADERS64* nt64 = get_nt_hdrs64(payload);
        image_size = nt64->OptionalHeader.SizeOfImage;
    } else {
        IMAGE_NT_HEADERS32* nt32 = get_nt_hdrs32(payload);
        image_size = nt32->OptionalHeader.SizeOfImage;
    }
    return image_size;
}


IMAGE_NT_HEADERS32* get_nt_hdrs32(IN const BYTE *payload)
{
    if (!payload) return nullptr;

    BYTE *ptr = get_nt_hdrs(payload);
    if (!ptr) return nullptr;

    if (!is64bit(payload)) {
        return (IMAGE_NT_HEADERS32*)ptr;
    }
    return nullptr;
}

IMAGE_NT_HEADERS64* get_nt_hdrs64(IN const BYTE *payload)
{
    if (payload == nullptr) return nullptr;

    BYTE *ptr = get_nt_hdrs(payload);
    if (!ptr) return nullptr;

    if (is64bit(payload)) {
        return (IMAGE_NT_HEADERS64*)ptr;
    }
    return nullptr;
}


bool has_relocations(IN const BYTE *pe_buffer)
{
    IMAGE_DATA_DIRECTORY* relocDir = get_directory_entry(pe_buffer, IMAGE_DIRECTORY_ENTRY_BASERELOC);
    if (!relocDir) {
        return false;
    }
    return true;
}