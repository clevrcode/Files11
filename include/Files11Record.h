#pragma once

#include <stdint.h>
#include <string>
#include <vector>
#include "Files11_defs.h"
#include "Files11Base.h"

class Files11Record : public Files11Base 
{
public:
	Files11Record(void);
	Files11Record(const Files11Record&);

	int Initialize(int lbn, std::ifstream& istrm);
	uint16_t GetFileNumber(void) const          { return fileNumber; };
	uint16_t GetFileSeq(void) const             { return fileSeq; };
	uint16_t GetFileRevision(void) const        { return fileRevision; };
	uint32_t GetHeaderLBN(void) const           { return headerLBN; };
	int      GetBlockCount(void) const          { return blockCount; };
	bool	 IsDirectory(void) const            { return bDirectory; };
	const char* GetFileName(void) const         { return fileName.c_str(); };
	const char* GetBlockCountString(void) const;
	const char* GetFileCreation(bool no_seconds = true) const;
	const char* GetFileExt(void) const          { return fileExt.c_str(); };
	const char* GetFullName(void) const         { return fullName.c_str(); };
	const char* GetFileRevisionDate(void) const {	return fileRevisionDate.c_str(); };
	const BlockList_t& getBlockList(void) const { return blockList; };
	std::string	fileName;

private:
	uint16_t	fileNumber;
	uint16_t	fileSeq;
	uint16_t	fileVersion;
	uint16_t	fileRevision;
	uint16_t	ownerUIC;
	uint16_t    fileProtection;
	uint8_t		sysCharacteristics;
	uint8_t		userCharacteristics;
	//ODS1_UserAttrArea_t	fileFCS;
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

