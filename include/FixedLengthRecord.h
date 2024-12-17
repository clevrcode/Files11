#pragma once
#include "Files11Base.h"

class FixedLengthRecord :
    public Files11Base
{
public:
    static int  CalculateFileLength(const char* fname);
    static bool IsFixedLengthRecordFile(const char* fname);
    static bool WriteFile(const char* fname, std::fstream& outFile, BlockList_t& blkList, ODS1_UserAttrArea_t* pUserAttr);
};

