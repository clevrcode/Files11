#include "ProductInfo.h"

#include "Windows.h"
#include <iostream>
#include <vector>

ProductInfo::ProductInfo()
{
    // get the filename of the executable containing the version resource
    TCHAR szFilename[MAX_PATH + 1] = { 0 };
    if (GetModuleFileName(nullptr, szFilename, MAX_PATH) == 0)
    {
        //TRACE("GetModuleFileName failed with error %d\n", GetLastError());
    }

    // allocate a block of memory for the version info
    DWORD dummy;
    DWORD dwSize = GetFileVersionInfoSize(szFilename, &dummy);
    if (dwSize == 0)
    {
        //TRACE("GetFileVersionInfoSize failed with error %d\n", GetLastError());
    }

    std::vector<BYTE> data(dwSize);
    // load the version info
    if (!GetFileVersionInfo(szFilename, 0, dwSize, &data[0]))
    {
        //TRACE("GetFileVersionInfo failed with error %d\n", GetLastError());
    }

    // get the name and version strings
    LPVOID pvProductName = nullptr;
    unsigned int iProductNameLen = 0;
    LPVOID pvProductVersion = nullptr;
    unsigned int iProductVersionLen = 0;

    struct LANGANDCODEPAGE {
        WORD wLanguage;
        WORD wCodePage;
    } *lpTranslate;
    UINT cbTranslate;

    // Read the list of languages and code pages.

    VerQueryValue(&data[0], TEXT("\\VarFileInfo\\Translation"), (LPVOID*)&lpTranslate, &cbTranslate);
    for (size_t i = 0; i < (cbTranslate / sizeof(struct LANGANDCODEPAGE)); i++)
    {
        char SubBlock[50];
        sprintf_s(SubBlock, sizeof(SubBlock), "\\StringFileInfo\\%04x%04x\\ProductName", lpTranslate[i].wLanguage, lpTranslate[i].wCodePage);
        // Retrieve file description for language and code page "i". 
        if (VerQueryValue(&data[0], SubBlock, &pvProductName, &iProductNameLen)) {
            sprintf_s(SubBlock, sizeof(SubBlock), "\\StringFileInfo\\%04x%04x\\ProductVersion", lpTranslate[i].wLanguage, lpTranslate[i].wCodePage);
            VerQueryValue(&data[0], SubBlock, &pvProductVersion, &iProductVersionLen);
            break;
        }
    }
    if (pvProductName != nullptr)
        strProductName = (const char*)pvProductName;       // , iProductNameLen);
    if (pvProductVersion != nullptr)
        strProductVersion = (const char*)pvProductVersion; // , iProductVersionLen);
}

void ProductInfo::PrintGreetings(void)
{
    std::cout << strProductName << " -- Version " << strProductVersion << std::endl;
}

void ProductInfo::PrintUsage(void)
{
    std::cout << "usage: >" << strProductName << " <disk - file>\n";
    std::cout << "        <disk-file> is a valid Files-11 disk image file\n";
}
