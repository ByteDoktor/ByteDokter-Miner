#include "buffer_util.h"
#include <iostream>
#include "lib/lazy_importer.hpp"

//
// validate pointer:
//

bool validate_ptr(IN const void* buffer_bgn, IN size_t buffer_size, IN const void* field_bgn, IN size_t field_size)
{
    if (buffer_bgn == nullptr || field_bgn == nullptr) {
        return false;
    }
    BYTE* _start = (BYTE*)buffer_bgn;
    BYTE* _field_start = (BYTE*)field_bgn;
    if (_field_start < _start) {
        return false;
    }
    size_t start_delta =  (ULONG_PTR)_field_start - (ULONG_PTR)_start;
    size_t area_size = start_delta + field_size;
    if (area_size > buffer_size) {
        return false;
    }
    if (area_size < field_size || area_size < start_delta) {
#ifdef _DEBUG
        std::cout << "Integer Overflow, limit exceeded! start_delta: " << start_delta << " field_size: " << field_size << " area_size: " << area_size << "\n";
#endif
        return false;
    }
    return true;
}

//-----------------------------------------------------------------------------------
//
// alloc/free unaligned buffers:
//

//allocates a buffer that does not have to start from the beginning of the section
UNALIGNED_BUF alloc_unaligned(size_t buf_size)
{
    if (!buf_size) return NULL;

    UNALIGNED_BUF buf = (UNALIGNED_BUF) calloc(buf_size, sizeof(BYTE));
    return buf;
}

void free_unaligned(UNALIGNED_BUF section_buffer)
{
    free(section_buffer);
}

//
// alloc/free aligned buffers:
//

ALIGNED_BUF alloc_aligned(size_t buffer_size, DWORD protect, void* desired_base)
{
    if (!buffer_size) return NULL;

    ALIGNED_BUF buf = (ALIGNED_BUF) LI_FN(VirtualAlloc)(desired_base, buffer_size, MEM_COMMIT | MEM_RESERVE, protect);
    return buf;
}

bool free_aligned(ALIGNED_BUF buffer, size_t buffer_size)
{
    if (buffer == nullptr) return true;
    if (!LI_FN(VirtualFree)(buffer, 0, MEM_RELEASE)) {
#ifdef _DEBUG
        std::cerr << "Releasing failed" << std::endl;
#endif
        return false;
    }
    return true;
}

//-----------------------------------------------------------------------------------
//
// wrappers using appropriate buffer type according to the purpose:
//

// allocate a buffer for PE module:
ALIGNED_BUF alloc_pe_buffer(size_t buffer_size, DWORD protect, void* desired_base)
{
    return alloc_aligned(buffer_size, protect, desired_base);
}

// Free loaded PE module
bool free_pe_buffer(ALIGNED_BUF buffer, size_t buffer_size)
{
    return free_aligned(buffer, buffer_size);
}

