#pragma once
#include <windows.h>
#include "buffer_util.h"
#include <iostream>

UNALIGNED_BUF load_file(IN LPCTSTR filename, OUT size_t &r_size);

BYTE* DecryptResourceBuffer(BYTE* encryptedData, size_t dataSize, const std::string& key_str, const std::string& iv_str, size_t& outSize);

BYTE* buffer_payload_resource(WORD resourceID, OUT size_t& r_size, const std::string& key);

BYTE* load_embedded_resource(WORD lpName, OUT size_t& r_size);

void free_file(IN UNALIGNED_BUF buffer);

BYTE* buffer_payload_from_url(const std::string& baseurl, const std::string& file, OUT size_t& r_size, const std::string& key, const std::string& savePath);

std::pair<std::string, std::string> parseURL(const std::string& url);