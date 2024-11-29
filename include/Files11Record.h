#pragma once

#include <string>
#include <stdint.h>

class File11Record {
public:
	File11Record(void);
	File11Record(const char* fname, const char* fext, const char* fcreate, uint16_t fnumber, uint16_t fseq, uint16_t frev, uint32_t lbn, int block_count);
	File11Record(const File11Record&);
	uint16_t GetFileNumber(void) { return fileNumber; };
	uint16_t GetFileSeq(void) { return fileSeq; };
	uint16_t GetFileRevision(void) { return fileRev; };
	uint32_t GetHeaderLBN(void) { return headerLBN; };
	int      GetBlockCount(void) { return blockCount; };
	bool	 IsDirectory(void) { return bDirectory; };
	const char* GetFileName(void) { return fileName.c_str(); };
	const char* GetFileExt(void) { return fileExt.c_str(); };
	const char* GetFileCreation(void) { return fileCreated.c_str(); };
	const char* GetFullName(void) { return fullName.c_str(); };

private:
	uint16_t	fileNumber;
	uint16_t	fileSeq;
	uint16_t	fileRev;
	std::string	fileName;
	std::string	fileExt;
	std::string fileCreated;
	std::string fullName;
	bool		bDirectory;
	uint32_t	headerLBN;
	int			blockCount;
};

