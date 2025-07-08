#include "pe_loader.h"
#include "pe_hdrs_helper.h"
#include "pe_helper.h"
#include "relocate.h"
#include "buffer_util.h"
#include "pe_raw_to_virtual.h"
#include "file_util.h"

BYTE* load_no_sec_pe(BYTE* dllRawData, size_t r_size, OUT size_t &v_size, bool executable)
{
    ULONG_PTR desired_base = 0;
    size_t out_size = (r_size < PAGE_SIZE) ? PAGE_SIZE : r_size;
    if (executable) {
        desired_base = get_image_base(dllRawData);
        out_size = get_image_size(dllRawData);
    }
    DWORD protect = (executable) ? PAGE_EXECUTE_READWRITE : PAGE_READWRITE;
    BYTE* mappedPE = alloc_pe_buffer(out_size, protect, reinterpret_cast<void*>(desired_base));
    if (!mappedPE) {
        return nullptr;
    }
    memcpy(mappedPE, dllRawData, r_size);
    v_size = out_size;
    return mappedPE;
}

BYTE* load_pe_module(BYTE* dllRawData, size_t r_size, OUT size_t &v_size, bool executable, bool relocate, ULONG_PTR desired_base)
{
    if (!get_nt_hdrs(dllRawData, r_size)) {
        return nullptr;
    }
    if (get_sections_count(dllRawData, r_size) == 0) {
        return load_no_sec_pe(dllRawData, r_size, v_size, executable);
    }
    // by default, allow to load the PE at the supplied base
    // if relocating is required, but the PE has no relocation table...
    if (relocate && !has_relocations(dllRawData)) {
        // ...enforce loading the PE image at its default base (so that it will need no relocations)
        desired_base = get_image_base(dllRawData);
    }
    // load a virtual image of the PE file at the desired_base address (random if desired_base is NULL):
    BYTE *mappedDLL = pe_raw_to_virtual(dllRawData, r_size, v_size, executable, desired_base);
    if (mappedDLL) {
        //if the image was loaded at its default base, relocate_module will return always true (because relocating is already done)
        if (relocate && !relocate_module(mappedDLL, v_size, (ULONGLONG)mappedDLL)) {
            // relocating was required, but it failed - thus, the full PE image is useless
            std::cerr << "[!] Could not relocate the module!\n";
            free_pe_buffer(mappedDLL, v_size);
            mappedDLL = nullptr;
        }
    } else {
        std::cerr << "[!] Could not allocate memory at the desired base!\n";
    }
    return mappedDLL;
}

BYTE* load_pe_module(LPCTSTR filename, OUT size_t &v_size, bool executable, bool relocate, ULONG_PTR desired_base)
{
    size_t r_size = 0;
    BYTE *dllRawData = load_file(filename, r_size);
    if (!dllRawData) {
#ifdef _DEBUG
        std::cerr << "Cannot load the file: " << filename << std::endl;
#endif
        return nullptr;
    }
    BYTE* mappedPE = load_pe_module(dllRawData, r_size, v_size, executable, relocate, desired_base);
    free_file(dllRawData);
    return mappedPE;
}


BYTE* load_pe_module_r(WORD resourcename, OUT size_t& v_size, bool executable,bool relocate, ULONG_PTR desired_base )
{
    // Load resource data.
    size_t r_size =  0;
    BYTE* dllRawData = load_embedded_resource(resourcename, r_size);
    if (!dllRawData) { return nullptr; }

    BYTE* mappedPE = load_pe_module(dllRawData, r_size, v_size, executable, relocate, desired_base);

    delete[] dllRawData;  // Remember to free allocated memory for raw data after usage.
    // The `load_file` function does not allocate any memory.

    return mappedPE;
}
