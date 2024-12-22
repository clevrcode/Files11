#pragma once
#include <string>
#include <vector>
#include <map>
#include <functional>

class DirDatabase
{
public:
	typedef struct DirFileRecord {
		DirFileRecord(void) : fnumber(0), version(0), fsequence(0) {};
		DirFileRecord(int nb, int seq, int ver, const std::string &name, const std::string &ext) : fnumber(nb), fsequence(seq),	version(ver), name(name), ext(ext) {};
		DirFileRecord(const struct DirFileRecord& rec) : fnumber(rec.fnumber), fsequence(rec.fsequence), name(rec.name), ext(rec.ext), version(rec.version) {};
		int         fnumber;
		int         fsequence;
		int         version;
		std::string name;
		std::string ext;
	} DirFileRecord_t;
	typedef std::vector<DirFileRecord_t> DirFileList_t;

	typedef struct DirInfo {
		DirInfo(void) : fnumber(0), lbn(0) {};
		DirInfo(int nb, int lbn) : fnumber(nb), lbn(lbn) { fileList.clear(); };
		DirInfo(const struct DirInfo& di) : fnumber(di.fnumber), lbn(di.lbn), fileList(di.fileList) {};
		void append(const DirFileRecord_t &rec) { fileList.push_back(rec); };
		int fnumber;
		int lbn;
		DirFileList_t fileList;
	} DirInfo_t;
	typedef std::vector<DirInfo_t> DirList_t;

	DirDatabase(void) {};
	bool Add(const std::string& name, DirInfo_t &info);
	bool Exist(const char* dname) const;
	int  Find(const char *dname, DirList_t& dlist) const;

	static std::string FormatDirectory(const std::string& dir);
	static bool isWildcard(const std::string& str);
	static int getUIC_hi(const std::string& in);
	static int getUIC_lo(const std::string& in);
	static std::string makeKey(const std::string& dir);

	void Populate(std::function<void(int, uint8_t*)> pf);

private:
	std::map<std::string, DirInfo_t> m_Database;
};

