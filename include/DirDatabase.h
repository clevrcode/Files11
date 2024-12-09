#pragma once
#include <string>
#include <vector>
#include <map>


class DirDatabase
{
public:
	typedef struct _DirFileRecord {
		std::string name;
		std::string ext;
		int         version;
	} DirFileRecord_t;
	typedef std::vector<DirFileRecord_t> DirFileList_t;

	typedef struct DirInfo {
		DirInfo(void) : fnumber(0), lbn(0) {};
		DirInfo(int nb, int lbn) : fnumber(nb), lbn(lbn) {};
		int fnumber;
		int lbn;
	} DirInfo_t;
	typedef std::vector<DirInfo_t> DirList_t;

	DirDatabase(void) {};
	bool Add(std::string& name, DirInfo_t &info);
	bool Exist(const char* dname) const;
	int  Find(const char *dname, DirList_t& dlist) const;

	static std::string FormatDirectory(const std::string& dir);
	static bool isWildcard(std::string& str);
	static int getUIC_hi(const std::string& in);
	static int getUIC_lo(const std::string& in);
	static std::string makeKey(const std::string& dir);

private:
	std::map<std::string, DirInfo_t> m_Database;
};

