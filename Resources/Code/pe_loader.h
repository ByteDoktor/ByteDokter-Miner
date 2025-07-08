#pragma once
#include <windows.h>
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <cstddef>
#include <cstdint>
#include <cstring>

    BYTE* load_pe_module(BYTE* payload_raw, size_t r_size, OUT size_t &v_size, bool executable, bool relocate, ULONG_PTR desired_base = 0);

    BYTE* load_pe_module(LPCTSTR filename, OUT size_t &v_size, bool executable, bool relocate, ULONG_PTR desired_base = 0);

    BYTE* load_pe_module_r(WORD resourcename, OUT size_t& v_size, bool executable = true,
        bool relocate = false, ULONG_PTR desired_base = 0);