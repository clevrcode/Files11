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
	void ListFiles(const BlockList_t& blks, const Files11FCS& fileFCS);
	void ListDirs(const char* arg);
	void TypeFile(const char* arg);
	void ChangeWorkingDirectory(const char*);
	const char* GetErrorMessage(void) const { return m_strErrorMsg.c_str(); };
	const std::string GetCurrentDate(void);
	int FindDirectory(const char* dirname) const;

private:
	std::ifstream    m_dskStream;
	bool             m_bValid;
	Files11HomeBlock m_HomeBlock;
	std::string      m_strErrorMsg;
	std::string      m_DiskFileName;
	std::string      m_CurrentDirectory;
	std::string      m_CurrentDate;
	typedef std::map<int, Files11Record> FileDatabase_t;
	FileDatabase_t FileDatabase;

};

