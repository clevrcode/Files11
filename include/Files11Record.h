#pragma once

#include <stdint.h>
#include <string>
#include <vector>
#include "Files11_defs.h"
#include "Files11Base.h"
#include "Files11FCS.h"

class Files11Record : public Files11Base 
{
public:
	Files11Record(int IndexLBN=0);
	Files11Record(const Files11Record&);

	int Initialize(int lbn, std::fstream& istrm);
	uint16_t GetFileNumber(void) const          { return fileNumber; };
	uint16_t GetFileSeq(void) const             { return fileSeq; };
	uint16_t GetFileRevision(void) const        { return fileRevision; };
	uint32_t GetHeaderLBN(void) const           { return headerLBN; };
	int      GetBlockCount(void) const          { return blockCount; };
	int      GetUsedBlockCount(void)            { return fileFCS.GetUsedBlockCount(); };
	bool	 IsDirectory(void) const            { return bDirectory; };
	bool     IsFileExtension(void) const        { return fileExtensionSegment != 0; };
	bool     IsContiguous(void) const           { return (userCharacteristics & uc_con) != 0; };
	const Files11FCS& GetFileFCS(void) const    { return fileFCS; };
	const char* GetFileName(void) const         { return fileName.c_str(); };
	const char* GetBlockCountString(void) const;
	const char* GetFileCreation(bool no_seconds = true) const;
	const char* GetFileExt(void) const          { return fileExt.c_str(); };
	const char* GetFullName(void) const         { return fullName.c_str(); };
	const char* GetFileRevisionDate(void) const { return fileRevisionDate.c_str(); };
	const BlockList_t& GetBlockList(void) const { return blockList; };
	const uint16_t GetOwnerUIC(void) const { return ownerUIC; };
	const uint16_t GetFileProtection(void) { return fileProtection; };
	bool ValidateHeader(ODS1_FileHeader_t* pHeader);
	ODS1_FileHeader_t* ReadFileHeader(int lbn, std::fstream& istrm);
	int BuildBlockList(int lbn, BlockList_t& blk_list, std::fstream& istrm);
	int GetBlockCount(F11_MapArea_t* pMap, BlockList_t* pBlkList = nullptr);


	std::string	fileName;

private:
	int         firstLBN;
	int         fileExtensionSegment;
	uint16_t	fileNumber;
	uint16_t	fileSeq;
	uint16_t	fileVersion;
	uint16_t	fileRevision;
	uint16_t	ownerUIC;
	uint16_t    fileProtection;
	uint8_t		sysCharacteristics;
	uint8_t		userCharacteristics;
	Files11FCS	fileFCS;
	std::string	fileExt;
	std::string fileCreationDate;
	std::string fileRevisionDate;
	std::string fileExpirationDate;
	std::string fullName;
	std::string blockCountString;
	bool		bDirectory;
	uint32_t	headerLBN;
	int			blockCount;
	BlockList_t blockList;
};

