#pragma once
#include "Files11Base.h"

class VarLengthRecord :
    public Files11Base
{
public:
    VarLengthRecord() {};

    static int  CalculateFileLength(const char *fname);
    static bool IsVariableLengthRecordFile(const char *fname);
    static bool WriteFile(const char *fname, std::fstream& outFile, BlockList_t& blkList, ODS1_UserAttrArea_t* pUserAttr);
};

