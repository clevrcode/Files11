#pragma once
#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include "Files11Base.h"
#include "Files11Record.h"
#include "Files11HomeBlock.h"
#include "FileDatabase.h"
#include "DirDatabase.h"

class Files11FileSystem : public Files11Base
{
public:
	Files11FileSystem();
	~Files11FileSystem();

	typedef struct FileInfo {
		FileInfo(void) : fnumber(0), version(0) {};
		FileInfo(int nb, int vrs) : fnumber(nb), version(vrs) {};
		int fnumber;
		int version;
	} FileInfo_t;
	typedef std::vector<FileInfo_t> FileList_t;
	typedef std::map<int, std::string> DirFileList_t;

	bool Open(const char* diskFile);
	void Close(void);
	void PrintVolumeInfo(void);
	int  GetDiskSize(void);
	bool BuildHeaderDatabase(void);
	bool BuildFileDatabase(void);
	void PrintFile(int fileNumber, std::ostream& strm);
	void DumpFile(int fileNumber, std::ostream& strm);
	int  GetHighestVersion(const char* dirname, const char* filename, Files11Record& fileRecord);
	int  GetDirFileList(const char* dirname, FileList_t &fileList);
	bool MarkDataBlock(BlockList_t blkList, bool used);
	bool MarkHeaderBlock(int lbn, bool used);
	bool AddDirectoryEntry(int lbn, DirectoryRecord_t* pDirEntry);

	typedef enum _Cmds {
		LIST = 1000,
		TYPE,
		EXPORT,
		IMPORT
	} Cmds_e;

	typedef std::vector<std::string> Args_t;
	// Commands
	void VerifyFileSystem(Args_t args);

	void ListFiles(const Files11Record& dirRecord, const char* filename, FileList_t &fileList);
	void ListDirs(Cmds_e cmd, const char* dir, const char *file);
	void TypeFile(const Files11Record& dirRecord, const char* filename);
	void ChangeWorkingDirectory(const char*);
	const char* GetCurrentWorkingDirectory(void) const { return m_CurrentDirectory.c_str(); };
	const char* GetErrorMessage(void) const { return m_strErrorMsg.c_str(); };
	const std::string GetCurrentDate(void);
	const std::string GetCurrentPDPTime(void);
	void PrintFreeBlocks(void);
	int FileNumberToLBN(int fnumber) const {
		if ((fnumber > 0) && (fnumber < m_FileNumberToLBN.size()))
			return m_FileNumberToLBN[fnumber];
		return -1;
	}

	bool AddFile(const char* nativeName, const char *pdp11Dir, const char* pdp11Name=nullptr);
	bool DeleteFile(const char* pdp11Dir, const char* pdp11name);
	bool DeleteFile(int fileNumber);

	int  FindFreeBlocks(int nbBlocks, BlockList_t &blkList);
	int  ValidateIndexBitmap(void);
	int  ValidateDirectory(const char* dirname, DirFileList_t & dirFileMap, int *pTotalFilesChecked); // This function is recursive
	int  ValidateStorageBitmap(void);

private:
	std::fstream     m_dskStream;
	bool             m_bValid;
	Files11HomeBlock m_HomeBlock;
	std::string      m_strErrorMsg;
	std::string      m_DiskFileName;
	std::string      m_CurrentDirectory;
	std::string      m_CurrentDate;
	std::string      m_CurrentTime;
	FileDatabase     FileDatabase;
	DirDatabase      DirDatabase;
	std::vector<int> m_FileNumberToLBN;

	//static void CountBits(uint8_t* blks, size_t nbBytes, int nbBlks, uint8_t lastByte, int& nbTrue, int& nbFalse, int& nbContiguous, int& largestContiguousBlock);

};

