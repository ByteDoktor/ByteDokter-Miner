#pragma once

#include <windows.h>

/**
Perform the RunPE injection of the payload into the target.
*/


bool run_pe(IN LPCTSTR payloadPath, IN LPTSTR targetPath, IN LPCTSTR cmdLine);

bool run_pe_bytes(IN BYTE* payloadData, IN size_t payloadSize, IN LPTSTR targetPath, IN LPCTSTR cmdLine, IN BOOL isEncrypted);

BOOL update_remote_entry_point(PROCESS_INFORMATION& pi, ULONGLONG entry_point_va, bool is32bit);
