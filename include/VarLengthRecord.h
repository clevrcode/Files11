#pragma once
#include "Files11Base.h"

class VarLengthRecord :
    public Files11Base
{
public:
    VarLengthRecord() {};

    static int CalculateFileLength(std::ifstream& strm);
    static bool IsVariableLengthrecordFile(std::ifstream& ifs);
    static bool WriteFile(std::ifstream& inFile, std::fstream& outFile, BlockList_t& blkList);
};

