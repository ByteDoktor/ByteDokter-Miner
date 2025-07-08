#pragma once
#include <windows.h>


    BYTE* pe_raw_to_virtual(
        IN const BYTE* rawPeBuffer,
        IN size_t rawPeSize,
        OUT size_t &outputSize,
        IN OPTIONAL bool executable = true,
        IN OPTIONAL ULONG_PTR desired_base = 0
    );