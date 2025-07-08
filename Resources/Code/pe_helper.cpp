#include "pe_helper.h"
#include "pe_hdrs_helper.h"



DWORD get_entry_point_rva(IN const BYTE *pe_buffer)
{
    //update image base in the written content:
    BYTE* payload_nt_hdr = get_nt_hdrs(pe_buffer);
    if (!payload_nt_hdr) {
        return 0;
    }
    const bool is64b = is64bit(pe_buffer);
    DWORD value = 0;
    if (is64b) {
        IMAGE_NT_HEADERS64* payload_nt_hdr64 = (IMAGE_NT_HEADERS64*)payload_nt_hdr;
        value = payload_nt_hdr64->OptionalHeader.AddressOfEntryPoint;
    } else {
        IMAGE_NT_HEADERS32* payload_nt_hdr32 = (IMAGE_NT_HEADERS32*)payload_nt_hdr;
        value = payload_nt_hdr32->OptionalHeader.AddressOfEntryPoint;
    }
    return value;
}

bool is64bit(IN const BYTE *pe_buffer)
{
    WORD arch = get_nt_hdr_architecture(pe_buffer);
    if (arch == IMAGE_NT_OPTIONAL_HDR64_MAGIC) {
        return true;
    }
    return false;
}