#pragma once
#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include "Files11Base.h"
#include "Files11Record.h"
#include "Files11HomeBlock.h"

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

	// Commands
	void RunCLI(void);
	void ListFiles(const BlockList_t& blks, const char* creationDate, int nbBlocks);
	void ListDirs(const char* arg);
	static void del(size_t n=1);
	static void putstring(const char* str);

	const char* GetErrorMessage(void) const { return m_strErrorMsg.c_str(); };

private:
	std::ifstream    m_dskStream;
	bool             m_bValid;
	Files11HomeBlock m_HomeBlock;
	std::string      m_strErrorMsg;
	std::string      m_DiskFileName;
	std::string      m_CurrentDirectory;

	typedef std::map<int, Files11Record> FileDatabase_t;
	FileDatabase_t FileDatabase;

};

