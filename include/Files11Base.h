#pragma once

#include <stdint.h>
#include <iostream>
#include <fstream>
#include "Files11_defs.h"


class Files11Base
{
public:
	Files11Base();
	~Files11Base();

protected:
	static bool     ReadBlock(int lbn, std::ifstream& istrm, uint8_t* buf);
	static uint16_t CalcChecksum(uint16_t* buffer, size_t wordCount);
	static void     MakeString(char* str, size_t len, std::string &outstr);
	static void     MakeDate(uint8_t* date, std::string& fdate, bool time);
	static void     MakeUIC(uint8_t* uic, std::string& strUIC);
};

