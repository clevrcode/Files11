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
	uint16_t GetFileNumber(void)      { return fileNumber; };
	uint16_t GetFileSeq(void)         { return fileSeq; };
	uint16_t GetFileRevision(void)    { return fileRevision; };
	uint32_t GetHeaderLBN(void)       { return headerLBN; };
	int      GetBlockCount(void)      { return blockCount; };
	bool	 IsDirectory(void)        { return bDirectory; };
	const char* GetFileName(void)     { return fileName.c_str(); };
	const char* GetFileExt(void)      { return fileExt.c_str(); };
	const char* GetFileCreation(void) { return fileCreationDate.c_str(); };
	const char* GetFullName(void)     { return fullName.c_str(); };

private:
	uint16_t	fileNumber;
	uint16_t	fileSeq;
	uint16_t	fileVersion;
	uint16_t	fileRevision;
	uint16_t	ownerUIC;
	uint16_t    fileProtection;
	uint8_t		sysCharacteristics;
	uint8_t		userCharacteristics;
	ODS1_UserAttrArea_t	fileFCS;
	std::string	fileName;
	std::string	fileExt;
	std::string fileCreationDate;
	std::string fileRevisionDate;
	std::string fileExpirationDate;
	std::string fullName;
	bool		bDirectory;
	uint32_t	headerLBN;

	int			blockCount;
	BlockList_t blockList;
};

