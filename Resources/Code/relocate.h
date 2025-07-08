#pragma once

#include <windows.h>    

#define RELOC_32BIT_FIELD 3
#define RELOC_64BIT_FIELD 0xA

typedef struct _BASE_RELOCATION_ENTRY {
    WORD Offset : 12;
    WORD Type : 4;
} BASE_RELOCATION_ENTRY;

class RelocBlockCallback
{
public:
    RelocBlockCallback(bool _is64bit)
        : is64bit(_is64bit)
    {
    }

    virtual bool processRelocField(ULONG_PTR relocField) = 0;

protected:
    bool is64bit;
};

bool process_relocation_table(IN PVOID modulePtr, IN SIZE_T moduleSize, IN RelocBlockCallback *callback);

bool relocate_module(IN BYTE* modulePtr, IN SIZE_T moduleSize, IN ULONGLONG newBase, IN ULONGLONG oldBase = 0);

bool apply_relocations(PVOID modulePtr, SIZE_T moduleSize, ULONGLONG newBase, ULONGLONG oldBase);

bool is_empty_reloc_block(BASE_RELOCATION_ENTRY *block, SIZE_T entriesNum, DWORD page, PVOID modulePtr, SIZE_T moduleSize);
bool process_reloc_block(BASE_RELOCATION_ENTRY *block, SIZE_T entriesNum, DWORD page, PVOID modulePtr, SIZE_T moduleSize, bool is64bit, RelocBlockCallback *callback);
