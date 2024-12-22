#pragma once
#include "Files11Base.h"

class VarLengthRecord :
    public Files11Base
{
public:
    static int  CalculateFileLength(const char *fname);
    static bool IsVariableLengthRecordFile(const char *fname);
    static bool WriteFile(const char *fname, std::fstream& outFile, BlockList_t& blkList, F11_UserAttrArea_t* pUserAttr);
    //static bool ExportFile(std::fstream& outFile, BlockList_t& blkList);
    static bool ValidateContent(void) {};
};

