#pragma once
#include <string>
#include <map>
#include "Files11Record.h"

class FileDatabase
{
public:
	FileDatabase(void);
	bool Add(int nb, Files11Record& frec);
	bool Get(int nb, Files11Record& frec);

private:
	typedef std::map<int, Files11Record> FileDatabase_t;
	FileDatabase_t m_Database;
};

