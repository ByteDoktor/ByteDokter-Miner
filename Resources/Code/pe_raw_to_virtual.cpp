#include "pe_raw_to_virtual.h"
#include "pe_hdrs_helper.h"
#include "buffer_util.h"
#include <iostream>
#include "pe_helper.h"

// Map raw PE into virtual memory of local process:
bool sections_raw_to_virtual(IN const BYTE* payload, IN SIZE_T payloadSize, OUT BYTE* destBuffer, IN SIZE_T destBufferSize)
{
    if (!payload || !destBuffer) return false;

    BYTE* payload_nt_hdr = get_nt_hdrs(payload, payloadSize);
    if (!payload_nt_hdr) {
        std::cerr << "Invalid PE: " << std::hex << (ULONGLONG) payload << std::endl;
        return false;
    }

    const bool is64b = is64bit(payload);

    IMAGE_FILE_HEADER *fileHdr = nullptr;
    DWORD hdrsSize = 0;
    void* secptr = nullptr;
    if (is64b) {
        IMAGE_NT_HEADERS64* payload_nt_hdr64 = (IMAGE_NT_HEADERS64*)payload_nt_hdr;
        fileHdr = &(payload_nt_hdr64->FileHeader);
        hdrsSize = payload_nt_hdr64->OptionalHeader.SizeOfHeaders;
        secptr = (void*)((ULONG_PTR)&(payload_nt_hdr64->OptionalHeader) + fileHdr->SizeOfOptionalHeader);
    }
    else {
        IMAGE_NT_HEADERS32* payload_nt_hdr32 = (IMAGE_NT_HEADERS32*)payload_nt_hdr;
        fileHdr = &(payload_nt_hdr32->FileHeader);
        hdrsSize = payload_nt_hdr32->OptionalHeader.SizeOfHeaders;
        secptr = (void*)((ULONG_PTR)&(payload_nt_hdr32->OptionalHeader) + fileHdr->SizeOfOptionalHeader);
    }
    DWORD first_raw = 0;
    //copy all the sections, one by one:
    for (WORD i = 0; i < fileHdr->NumberOfSections; i++) {
        PIMAGE_SECTION_HEADER next_sec = (PIMAGE_SECTION_HEADER)((ULONG_PTR)secptr + ((ULONG_PTR)IMAGE_SIZEOF_SECTION_HEADER * i));
        if (!validate_ptr(static_cast<const void*>(payload), payloadSize, next_sec, IMAGE_SIZEOF_SECTION_HEADER) // check if fits in the source size
            || !validate_ptr(static_cast<const void*>(payload), destBufferSize, next_sec, IMAGE_SIZEOF_SECTION_HEADER)) // check if fits in the destination size
        {
            return false;
        }
        if (next_sec->PointerToRawData == 0 || next_sec->SizeOfRawData == 0) {
            continue; //skipping empty
        }
        void* section_mapped = destBuffer + next_sec->VirtualAddress;
        void* section_raw_ptr = (BYTE*)payload +  next_sec->PointerToRawData;
        size_t sec_size = next_sec->SizeOfRawData;
        
        if ((next_sec->VirtualAddress + sec_size) > destBufferSize) {
            std::cerr << "[!] Virtual section size is out ouf bounds: " << std::hex << sec_size << std::endl;
            sec_size = (destBufferSize > next_sec->VirtualAddress) ? SIZE_T(destBufferSize - next_sec->VirtualAddress) : 0;
            std::cerr << "[!] Truncated to maximal size: " << std::hex << sec_size << ", buffer size:" << destBufferSize << std::endl;
        }
        if (next_sec->VirtualAddress >= destBufferSize && sec_size != 0) {
            std::cerr << "[-] VirtualAddress of section is out ouf bounds: " << std::hex << next_sec->VirtualAddress << std::endl;
            return false;
        }
        if (next_sec->PointerToRawData + sec_size > destBufferSize) {
            std::cerr << "[-] Raw section size is out ouf bounds: " << std::hex << sec_size << std::endl;
            return false;
        }

        // validate source:
        if (!validate_ptr(static_cast<const void*>(payload), payloadSize, section_raw_ptr, sec_size)) {
            if (next_sec->PointerToRawData > payloadSize) {
                std::cerr << "[-] Section " << i << ":  out ouf bounds, skipping... " << std::endl;
                continue;
            }
            // trim section
            sec_size = payloadSize - (next_sec->PointerToRawData);
        }
        // validate destination:
        if (!validate_ptr(destBuffer, destBufferSize, section_mapped, sec_size)) {
            std::cerr << "[-] Section " << i << ":  out ouf bounds, skipping... " << std::endl;
            continue;
        }
        memcpy(section_mapped, section_raw_ptr, sec_size);
        if (first_raw == 0 || (next_sec->PointerToRawData < first_raw)) {
            first_raw = next_sec->PointerToRawData;
        }
    }

    //copy payload's headers:
    if (hdrsSize == 0) {
        hdrsSize= first_raw;
#ifdef _DEBUG
        std::cout << "hdrsSize not filled, using calculated size: " << std::hex << hdrsSize << "\n";
#endif
    }
    if (!validate_ptr((const LPVOID)payload, destBufferSize, (const LPVOID)payload, hdrsSize)) {
        return false;
    }
    memcpy(destBuffer, payload, hdrsSize);
    return true;
}

BYTE* pe_raw_to_virtual(
    IN const BYTE* payload,
    IN size_t in_size,
    OUT size_t &out_size,
    IN OPTIONAL bool executable,
    IN OPTIONAL ULONG_PTR desired_base
)
{
    //check payload:
    BYTE* nt_hdr = get_nt_hdrs(payload);
    if (!nt_hdr) {
        std::cerr << "Invalid PE: " << std::hex << (ULONG_PTR) payload << std::endl;
        return nullptr;
    }
    DWORD payloadImageSize = 0;

    const bool is64 = is64bit(payload);
    if (is64) {
        IMAGE_NT_HEADERS64* payload_nt_hdr = (IMAGE_NT_HEADERS64*)nt_hdr;
        payloadImageSize = payload_nt_hdr->OptionalHeader.SizeOfImage;
    }
    else {
        IMAGE_NT_HEADERS32* payload_nt_hdr = (IMAGE_NT_HEADERS32*)nt_hdr;
        payloadImageSize = payload_nt_hdr->OptionalHeader.SizeOfImage;
    }
    payloadImageSize = round_up_to_unit(payloadImageSize, (DWORD)PAGE_SIZE);

    DWORD protect = executable ? PAGE_EXECUTE_READWRITE : PAGE_READWRITE;
    //first we will prepare the payload image in the local memory, so that it will be easier to edit it, apply relocations etc.
    //when it will be ready, we will copy it into the space reserved in the target process
    BYTE* localCopyAddress = alloc_pe_buffer(payloadImageSize, protect, reinterpret_cast<void*>(desired_base));
    if (!localCopyAddress) {
        std::cerr << "Could not allocate memory in the current process" << std::endl;
        return NULL;
    }
    //printf("Allocated local memory: %p size: %x\n", localCopyAddress, payloadImageSize);
    if (!sections_raw_to_virtual(payload, in_size, localCopyAddress, payloadImageSize)) {
        std::cerr <<  "Could not copy PE file" << std::endl;
        return nullptr;
    }
    out_size = payloadImageSize;
    return localCopyAddress;
}
