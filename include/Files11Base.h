#pragma once

#include <stdint.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include "Files11_defs.h"


class Files11Base
{
public:
	Files11Base();
	uint8_t *ReadBlock(int lbn, std::ifstream& istrm);
	F11_MapArea_t* GetMapArea(void) const;
	F11_IdentArea_t* GetIdentArea(void) const;

	struct BlockPtrs {
		BlockPtrs() : lbn_start(0), lbn_end(0) {};
		uint32_t  lbn_start;
		uint32_t  lbn_end;
	};
	typedef struct BlockPtrs BlockPtrs_t;
	typedef std::vector<BlockPtrs_t> BlockList_t;

protected:
	
	static uint16_t CalcChecksum(uint16_t* buffer, size_t wordCount);
	static void     MakeString(char* str, size_t len, std::string &outstr, bool strip=false);
	static void     MakeDate(uint8_t* date, std::string& fdate, bool time);
	static void     MakeUIC(uint8_t* uic, std::string& strUIC);
	static void     Radix50ToAscii(uint16_t* pR50, int len, std::string& str, bool strip=false);
	static uint8_t *readBlock(int lbn, std::ifstream& istrm, uint8_t*blk);

private:
	uint8_t m_block[F11_BLOCK_SIZE];
};

