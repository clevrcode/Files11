#pragma once
#include <string>
#include <map>
#include "Files11Record.h"

class FileDatabase
{
public:
	FileDatabase(int maxFile=0) : m_MaxFileNumber(maxFile) {};
	void SetMaxFile(int maxFile) { m_MaxFileNumber = maxFile; };
	bool Add(int nb, const Files11Record& frec);
	bool Exist(int nb) const;
	bool Get(int nb, Files11Record& frec);
	bool Get(int nb, Files11Record& frec, int version, const char *filter);
	bool Delete(int nb);
	bool Filter(const Files11Record& rec, const char* name);
	int  GetNbHeaders(void) const { return (int)m_Database.size(); };
	static void SplitName(const std::string &fullname, std::string& name, std::string& ext, std::string& version);
	int  FindFirstFreeFile(void);

private:
	typedef std::map<int, Files11Record> FileDatabase_t;
	FileDatabase_t m_Database;
	int m_MaxFileNumber;
};

