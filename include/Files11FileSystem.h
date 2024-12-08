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

	bool Open(const char* diskFile);
	void Close(void);
	void PrintVolumeInfo(void);
	int  GetDiskSize(void);
	bool BuildHeaderDatabase(void);
	bool BuildFileDatabase(void);
	void PrintFile(int fileNumber, std::ostream& strm);
	void DumpFile(int fileNumber, std::ostream& strm);

	typedef enum _Cmds {
		LIST = 1000,
		TYPE,
		EXPORT,
		IMPORT
	} Cmds_e;

	// Commands
	void ListFiles(const BlockList_t& dirblks, const Files11FCS& dirFCS, const char* filename);
	void ListDirs(Cmds_e cmd, const char* dir, const char *file);
	void TypeFile(const BlockList_t& dirblks, const Files11FCS& dirFCS, const char* filename);
	void ChangeWorkingDirectory(const char*);
	const char* GetCurrentWorkingDirectory(void) const { return m_CurrentDirectory.c_str(); };
	const char* GetErrorMessage(void) const { return m_strErrorMsg.c_str(); };
	const std::string GetCurrentDate(void);
	void PrintFreeBlocks(void);
	//int FindFile(const char* path_name, std::vector<int> &flist) const;

	bool AddFile(const char* nativeName, const char *pdp11Dir, const char* pdp1Name);
	int  FindFreeFile(void);
	int  FindFreeBlocks(int nbBlocks);

private:
	std::fstream     m_dskStream;
	bool             m_bValid;
	Files11HomeBlock m_HomeBlock;
	std::string      m_strErrorMsg;
	std::string      m_DiskFileName;
	std::string      m_CurrentDirectory;
	std::string      m_CurrentDate;
	FileDatabase     FileDatabase;
	DirDatabase      DirDatabase;

	static void CountBits(uint8_t* blks, size_t nbBytes, int nbBlks, uint8_t lastByte, int& nbTrue, int& nbFalse, int& nbContiguous, int& largestContiguousBlock);

};

