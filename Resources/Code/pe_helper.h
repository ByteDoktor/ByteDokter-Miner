#pragma once

#include <windows.h>

DWORD get_entry_point_rva(IN const BYTE *pe_buffer);

bool is64bit(IN const BYTE *pe_buffer);