#pragma once
#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include "Files11Base.h"
#include "Files11Record.h"
#include "Files11HomeBlock.h"
#include "FileDatabase.h"

#if defined(DeleteFile)
#undef DeleteFile
#endif


class Files11FileSystem
{
public:
	Files11FileSystem();
	~Files11FileSystem();

	const char* ALL_FILES = "*.*;*";
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
	void PrintFile(int fileNumber, std::ostream& strm);
	void DumpFile(int fileNumber, std::ostream& strm);
	void DumpHeader(int fileNumber);
	void SplitFilePath(const std::string& path, std::string& dir, std::string& file);
	int  GetHighestVersion(int dirfnb, const char* filename, Files11Record& fileRecord);
	int  GetDirList(const char* dirname, FileDatabase::DirList_t &dlist);
	int  GetFileList(int dirfnb, FileList_t& fileList);
	bool MarkDataBlock(Files11Base::BlockList_t blkList, bool used);
	bool MarkHeaderBlock(int lbn, bool used);
	bool AddDirectoryEntry(int filenb, DirectoryRecord_t* pDirEntry);
	bool DeleteDirectoryEntry(int filenb, std::vector<int> &fileNbToRemove);

	typedef enum _Cmds {
		LIST = 1000,
		TYPE,
		DMPHDR,
		EXPORT,
		IMPORT
	} Cmds_e;

	typedef std::vector<std::string> Args_t;
	// Commands
	void VerifyFileSystem(Args_t args);

	//void ListFiles(const Files11Record& dirRecord, const char* filename, FileList_t &fileList);
	void ListFiles(const Args_t& args);
	void ListDirs(const Args_t &args);
	bool DeleteFile(const Args_t& args);
	void DumpLBN(const Args_t &args);
	void DumpHeader(const Args_t &args);
	void FullList(const Args_t& args);
	//void DumpHeader(const Files11Record& dirRecord, const char* filename);

	void ExportFiles(const Args_t& args);
	void ExportDirectory(Files11Record &dirInfo, const char *fnameFilter=nullptr);

	//void TypeFile(const Files11Record& dirRecord, const char* filename);
	void TypeFile(int fnumber);


	void ChangeWorkingDirectory(const char*);
	const char* GetCurrentWorkingDirectory(void) const { return m_CurrentDirectory.c_str(); };
	const char* GetErrorMessage(void) const { return m_strErrorMsg.c_str(); };
	const std::string GetCurrentSystemTime(void) { return m_File.GetCurrentPDPTime(); };
	void PrintFreeBlocks(void);

	bool AddFile(const Args_t& args, const char* nativeName);
	bool DeleteFile(int fileNumber);

	int  FindFreeBlocks(int nbBlocks, Files11Base::BlockList_t &blkList);
	int  ValidateIndexBitmap(int *nbIndexBlockUsed);
	int  ValidateDirectory(const char* dirname, DirFileList_t & dirFileMap, int *pTotalFilesChecked); // This function is recursive
	int  ValidateStorageBitmap(int* nbBitmapBlockUsed);
	static void print_error(const char* msg);

private:
	static void print_blocks(int blk_hi, int blk_lo);
	static void print_map_area(F11_MapArea_t* pMap);

	int GetBlockList(int lbn, Files11Base::BlockList_t &blkList);
	int BuildBlockList(int lbn, Files11Base::BlockList_t& blk_list, std::fstream& istrm);

	Files11Base      m_File;
	std::fstream     m_dskStream;
	bool             m_bValid;
	Files11HomeBlock m_HomeBlock;
	std::string      m_strErrorMsg;
	std::string      m_DiskFileName;
	std::string      m_CurrentDirectory;
	FileDatabase     FileDatabase;
	std::vector<int> m_LBNtoBitmapPage;

	//std::vector<int> m_FileNumberToLBN;
};

