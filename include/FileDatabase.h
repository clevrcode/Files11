#pragma once
#include <string>
#include <map>
#include <functional>
#include "Files11Record.h"

class DiskBlock
{
public:
	DiskBlock(void) : first(-1), last(-1) {};
	DiskBlock(const char* name, int first, int size) : filename(name), first(first) { last = first + size - 1; };
	bool IsFreeBlock(void) { return filename.empty(); };

private:
	std::string filename;
	int first;
	int last;
};

class FileDatabase
{
public:
	FileDatabase(int maxFile=0) : m_MaxFileNumber(maxFile), m_TotalBlock(0) {};
	void SetMaxFile(int maxFile, int totalBlk) { m_MaxFileNumber = maxFile; m_TotalBlock = totalBlk; };
	void Add(int nb, const Files11Record& frec);
	bool Exist(int nb) const;
	bool Get(int nb, Files11Record& frec);
	bool Get(int nb, Files11Record& frec, const char *filter);
	bool Delete(int nb);
	bool Filter(const Files11Record& rec, const char* name);
	int  GetNbHeaders(void) const { return (int)m_Database.size(); };
	int  FindFirstFreeFile(void);
	int  FileNumberToLBN(int fnb) { return (fnb < m_FileNumberToLBN.size()) ? m_FileNumberToLBN[fnb] : -1; };
	void SetLBN(int fnb, int lbn) { m_FileNumberToLBN[fnb] = lbn; };
	void AppendLBN(int lbn)       { m_FileNumberToLBN.push_back(lbn); };
	static void SplitName(const std::string &fullname, std::string& name, std::string& ext, std::string& version);

	// Directory stuff
	static std::string FormatDirectory(const std::string& dir);
	static bool isWildcard(const std::string& str);
	static int getUIC_hi(const std::string& in);
	static int getUIC_lo(const std::string& in);
	static std::string makeKey(const std::string& dir);

	typedef struct DirFileRecord {
		DirFileRecord(void) : fnumber(0), version(0), fsequence(0) {};
		DirFileRecord(int nb, int seq, int ver, const std::string& name, const std::string& ext) : fnumber(nb), fsequence(seq), version(ver), name(name), ext(ext) {};
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
		void append(const DirFileRecord_t& rec) { fileList.push_back(rec); };
		int fnumber;
		int lbn;
		DirFileList_t fileList;
	} DirInfo_t;
	typedef std::vector<DirInfo_t> DirList_t;

	int  FindDirectory(const char* dname, DirList_t& dlist) const;
	void FindMatchingFiles(const char* dir, const char* filename, DirList_t &dirList, std::function<void(int, Files11Base& obj)> fetch);

	// Directory methods
	bool DirectoryExist(const char* dname) const;

private:
	typedef std::map<int, Files11Record> FileDatabase_t;
	typedef std::map<std::string, DirInfo_t> DirDatabase_t;
	FileDatabase_t   m_Database;
	DirDatabase_t    m_DirDatabase;
	std::vector<int> m_FileNumberToLBN;
	int              m_MaxFileNumber;
	int              m_TotalBlock;
};

