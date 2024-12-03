#pragma once
#include <string>
#include <vector>
#include <map>

typedef std::vector<int> DirList_t;

class DirDatabase
{
public:
	DirDatabase(void);
	bool Add(std::string& name, int fnumber);
	bool Exist(const char* dname) const;
	int  Find(const char *dname, DirList_t& dlist) const;

	static std::string FormatDirectory(const std::string& dir);
	static bool isWildcard(std::string& str);
	static int getUIC_hi(const std::string& in);
	static int getUIC_lo(const std::string& in);
	static std::string makeKey(const std::string& dir);
private:
	std::map<std::string, int> m_Database;
};

