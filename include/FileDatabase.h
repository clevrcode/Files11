#pragma once
#include <string>
#include <map>
#include "Files11Record.h"

class FileDatabase
{
public:
	FileDatabase(void);
	bool Add(int nb, const Files11Record& frec);
	bool Get(int nb, Files11Record& frec, const char *filter);
	bool Filter(const Files11Record& rec, const char* name) const;
private:
	typedef std::map<int, Files11Record> FileDatabase_t;
	FileDatabase_t m_Database;
};

