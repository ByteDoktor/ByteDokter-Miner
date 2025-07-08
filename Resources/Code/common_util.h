#pragma once
#include <iostream>
#include <filesystem>
#include <wchar.h>
#include <vector>
#include <string>
#include <winternl.h>

bool DisableSleep();

void run_program(const char* programPath, const char* cmdLine);

bool runProcess(const std::string& commandLine);

char manualXOR(char a, char b);

void StringToWString(std::wstring &ws, const std::string &s);

DWORD splitFirstSpace(LPCTSTR str);

std::wstring ConvertToWide(LPCTSTR cmdLine);

std::vector<uint8_t> toUTF16LE(const std::wstring& input);

void writeAllBytes(const std::string& filename, const std::vector<char>& data);

std::vector<char> readAllBytes(const std::string& filename);

bool fileExists(const std::string& filename);

bool AddRegistryValue(const std::string &startupPath, const char* valueName);


bool directoryExists(const std::string& path);

bool createDirectory(const std::string& path);

bool endsWithExe(const std::string& path);

std::vector<std::string> splitPath(const std::string& path);

bool createPathRecursively(std::string fullPath);

bool InstallCheck();

bool mutexExists(const std::string& name);

bool IsAdmin();

std::string toLower(const std::string& str);

std::string normalizeSlashes(const std::string& path);
std::string normalizePath(const std::string& rawPath);

bool arePathsEqual(const char* path1, const std::string& path2);