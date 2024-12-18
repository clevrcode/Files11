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

	struct BlockPtrs {
		BlockPtrs() : lbn_start(0), lbn_end(0) {};
		BlockPtrs(int b_start, int b_end) : lbn_start(b_start), lbn_end(b_end) {};
		uint32_t  lbn_start;
		uint32_t  lbn_end;
	};
	typedef struct BlockPtrs BlockPtrs_t;
	typedef std::vector<BlockPtrs_t> BlockList_t;

	void                 ClearBlock(void);
	uint8_t             *ReadBlock(int lbn, std::fstream& istrm);
	bool                 WriteBlock(std::fstream& istrm);
	bool                 WriteHeader(std::fstream& istrm);
	ODS1_FileHeader_t*   ReadHeader(int lbn, std::fstream& istrm, bool clear=false);
	DirectoryRecord_t*   ReadDirectory(int lbn, std::fstream& istrm, bool clear=false);
	F11_MapArea_t*       GetMapArea(ODS1_FileHeader_t* ptr=nullptr) const;
	F11_IdentArea_t*     GetIdentArea(ODS1_FileHeader_t* ptr=nullptr) const;
	ODS1_UserAttrArea_t* GetUserAttr(ODS1_FileHeader_t* ptr=nullptr) const;
	bool                 CreateExtensionHeader(int lbn, int extFileNumber, ODS1_FileHeader_t* pHeader, BlockList_t &blkList, std::fstream& istrm);
	
	static uint16_t      CalcChecksum(uint16_t* buffer, size_t wordCount);
	static void          MakeString(char* str, size_t len, std::string &outstr, bool strip=false);
	static void          MakeDate(uint8_t* date, std::string& fdate, bool time);
	static void          MakeTime(uint8_t* tim, std::string& ftime);
	static void          MakeUIC(uint8_t* uic, std::string& strUIC);
	static void          FillDate(char *pDate, char *time=nullptr);
	static void          Radix50ToAscii(uint16_t* pR50, int len, std::string& str, bool strip=false);
	static int           GetRadix50Char(char c);
	static void          AsciiToRadix50(const char* src, size_t srclen, uint16_t* dest); // For each 3 src chars -> 1 16 bits word
	static void          PrintError(const char* dir, DirectoryRecord_t* p, const char* msg);
	const std::string    GetCurrentDate(void);
	const std::string    GetCurrentPDPTime(void);
	const std::string    GetFileProtectionString(uint16_t pro);

	static uint8_t      *readBlock(int lbn, std::fstream& istrm, uint8_t*blk);
	static uint8_t      *writeBlock(int lbn, std::fstream& istrm, uint8_t* blk);
	static bool          writeHeader(int lbn, std::fstream& istrm, ODS1_FileHeader_t* pHeader);

private:
	int                m_LastBlockRead;
	uint8_t            m_block[F11_BLOCK_SIZE];
	std::string        m_CurrentDate;
	std::string        m_CurrentTime;
	std::string        m_FileProtection;
	static const char* months[];
	static const char* radix50Chars;
};

