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

class Files11FileSystem
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
	void PrintFile(int fileNumber, std::ostream& strm);
	void DumpFile(int fileNumber, std::ostream& strm);
	int  GetHighestVersion(const char* dirname, const char* filename, Files11Record& fileRecord);
	int  GetDirFileList(const char* dirname, FileList_t &fileList);
	bool MarkDataBlock(Files11Base::BlockList_t blkList, bool used);
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
	void ExportFiles(const char* dirname, const char* filename, const char* outdir);

	void TypeFile(const Files11Record& dirRecord, const char* filename);
	void ChangeWorkingDirectory(const char*);
	const char* GetCurrentWorkingDirectory(void) const { return m_CurrentDirectory.c_str(); };
	const char* GetErrorMessage(void) const { return m_strErrorMsg.c_str(); };
	const std::string GetCurrentSystemTime(void) { return m_File.GetCurrentPDPTime(); };
	void PrintFreeBlocks(void);
	int FileNumberToLBN(int fnumber) const {
		if ((fnumber > 0) && (fnumber < m_FileNumberToLBN.size()))
			return m_FileNumberToLBN[fnumber];
		return -1;
	}

	void DumpLBN(int lbn);
	void DumpHeader(int fileNumber);

	bool AddFile(const char* nativeName, const char *pdp11Dir, const char* pdp11Name=nullptr);
	bool DeleteFile(const char* pdp11Dir, const char* pdp11name);
	bool DeleteFile(int fileNumber);

	int  FindFreeBlocks(int nbBlocks, Files11Base::BlockList_t &blkList);
	bool GetFreeBlocks(int first_lbn, int nb_blocks);
	int  ValidateIndexBitmap(void);
	int  ValidateDirectory(const char* dirname, DirFileList_t & dirFileMap, int *pTotalFilesChecked); // This function is recursive
	int  ValidateStorageBitmap(void);

private:

	int GetBlockList(int lbn);
	int GetBlockList(int lbn, Files11Base::BlockList_t &blkList);
	int BuildBlockList(int lbn, Files11Base::BlockList_t* blk_list, std::fstream& istrm);
	int GetBlockCount(F11_MapArea_t* pMap, Files11Base::BlockList_t* pBlkList = nullptr);
	Files11Base      m_File;
	std::fstream     m_dskStream;
	bool             m_bValid;
	Files11HomeBlock m_HomeBlock;
	std::string      m_strErrorMsg;
	std::string      m_DiskFileName;
	std::string      m_CurrentDirectory;
	FileDatabase     FileDatabase;
	DirDatabase      DirDatabase;
	std::vector<int> m_FileNumberToLBN;
};

