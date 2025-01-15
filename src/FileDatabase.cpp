#include <regex>
#include "FileDatabase.h"

void FileDatabase::Add(int nb, const Files11Record &frec)
{
    // Add a new entry for this file (key: file number)
    m_Database[nb] = frec;
    // If a directory, add to the directory database (key: dir name)
    if (frec.IsDirectory()) {
        DirInfo_t info(nb, frec.GetHeaderLBN());
        m_DirDatabase[frec.GetFileName()] = info;
    }
}

bool FileDatabase::Delete(int nb)
{
	if (!Exist(nb))
		return false;

	if (m_Database[nb].IsDirectory()) {
		m_DirDatabase.erase(m_Database[nb].GetFileName());
	}
    m_Database.erase(nb);
    return true;
}

int  FileDatabase::FindFirstFreeFile(void)
{
    int fileNumber = -1;
    for (int fnb = F11_CORIMG_SYS + 1; (fnb < m_MaxFileNumber) && (fileNumber == 0); ++fnb) {
        if (!Exist(fnb)) {
            // Mark it as used with an empty record
            Files11Record frec;
            m_Database[fnb] = frec;
            fileNumber = fnb;
            break;
        }
    }
    return fileNumber;
}

bool FileDatabase::Exist(int nb) const
{
    return m_Database.find(nb) != m_Database.end();
}

bool FileDatabase::Get(int nb, Files11Record& frec)
{
    auto rec = m_Database.find(nb);
    if (rec != m_Database.end()) {
        frec = rec->second;
        return true;
    }
    return false;
}

// -----------------------------------------------------------------------------
// Return a file record in frec if file name and version passes the filter

bool FileDatabase::Get(int nb, Files11Record& frec, const char *fname)
{
	if (!Exist(nb))
		return false;
    Get(nb, frec);
    return Filter(frec, fname);
}

void FileDatabase::SplitName(const std::string &fullname, std::string& name, std::string& ext, std::string& version)
{
    auto pos = fullname.find(".");
    if (pos == std::string::npos) {
        name = fullname;
        return;
    }
    name = fullname.substr(0,pos);
    ext = fullname.substr(pos + 1);
    pos = ext.find(";");
    if (pos == std::string::npos) {
        return;
    }
    version = ext.substr(pos + 1);
    ext = ext.substr(0, pos);
    return;
}

bool FileDatabase::Filter(const Files11Record& rec, const char* fullname)
{
    if (fullname != nullptr)
    {
        int iVersion = 0;
        const std::string strFullName(fullname);
        std::string name, ext, fversion;
        SplitName(strFullName, name, ext, fversion);

        auto pos = name.find("*");
        if (pos != std::string::npos)
        {
            std::string tmp(rec.GetFileName());
            if (name.substr(0, pos) == tmp.substr(0, pos))
                name = tmp;
        }
        pos = ext.find("*");
        if (pos != std::string::npos)
        {
            std::string tmp(rec.GetFileExt());
            if (ext.substr(0, pos) == tmp.substr(0, pos))
                ext = tmp;
        }
        if (fversion.length() > 0) {
            if (fversion == "*")
                iVersion = rec.GetFileVersion();
            else
                iVersion = strtol(fversion.c_str(), nullptr, 10);
            return (name == rec.GetFileName()) && (ext == rec.GetFileExt()) && (iVersion == rec.GetFileVersion());
        }
        return (name == rec.GetFileName()) && (ext == rec.GetFileExt());
    }
    return true;
}

//-------------------------------------------------------------------------


bool FileDatabase::DirectoryExist(const char* dname) const
{
    std::string strDirName(makeKey(dname));
    auto pos = m_DirDatabase.find(strDirName);
    return pos != m_DirDatabase.end();
}

int FileDatabase::FindDirectory(const char* dname, DirList_t& dlist) const
{
    dlist.clear();

    // Check if dname contains wildcard
    std::string dirname(FormatDirectory(dname));
    if (isWildcard(dirname))
    {
        if ((dirname == "[*]") || (dirname == "[*,*]"))
        {
            // return all directories (Except MFD 000000.DIR which contains all directories)
            for (auto& dir : m_DirDatabase) {
                if (dir.second.fnumber != F11_000000_SYS)
                    dlist.push_back(dir.second);
            }
        }
        else
        {
            std::smatch sm;
            std::regex_match(dirname, sm, std::regex("\\[\\*,([0-7]+)\\]"));
            if (sm.size() == 2)
            {
                int a = strtol(sm.str(1).c_str(), nullptr, 8);
                // return all directories
                for (auto& dir : m_DirDatabase)
                {
                    if (dir.second.fnumber != F11_000000_SYS) {
                        int b = getUIC_lo(dir.first);
                        if (a == b)
                            dlist.push_back(dir.second);
                    }
                }
            }
            else
            {
                std::regex_match(dirname, sm, std::regex("\\[([0-7]+),\\*\\]"));
                if (sm.size() == 2)
                {
                    int a = strtol(sm.str(1).c_str(), nullptr, 8);
                    // return all directories
                    for (auto& dir : m_DirDatabase)
                    {
                        if (dir.second.fnumber != F11_000000_SYS) {
                            int b = getUIC_hi(dir.first);
                            if (a == b)
                                dlist.push_back(dir.second);
                        }
                    }
                }
            }
        }
    }
    else
    {
        std::string key(makeKey(dirname));
        auto pos = m_DirDatabase.find(key);
        if (pos != m_DirDatabase.end())
            dlist.push_back(pos->second);
    }
    // Return the number of directories found
    return (int)dlist.size();;
}

std::string FileDatabase::FormatDirectory(const std::string& dir)
{
    std::string out(dir);
    if ((dir.length() == 6) && (dir.front() != '['))
    {
        std::regex re("([0-7]{3})([0-7]{3})");
        std::smatch sm;
        std::regex_match(dir, sm, re);
        if (sm.size() == 3)
        {
            out = "[" + sm.str(1) + "," + sm.str(2) + "]";
        }
    }
    else if ((dir.length() > 0) && (dir.front() == '[') && (dir.back() == ']'))
    {
        std::regex re("\\[([0-7]+),([0-7]+)\\]");
        std::smatch sm;
        std::regex_match(dir, sm, re);
        if (sm.size() == 3)
        {
            char buf[16];
            int a = strtol(sm.str(1).c_str(), nullptr, 8);
            int b = strtol(sm.str(2).c_str(), nullptr, 8);
            sprintf_s(buf, sizeof(buf), "[%03o,%03o]", a, b);
            out = buf;
        }
    }
    else if ((dir.length() > 0) && (dir.front() != '[') && (dir.back() != ']'))
    {
        auto pos = dir.find(".");
        if (pos != std::string::npos) {
            if (dir.substr(pos + 1, 3) == "DIR")
                out = "[" + dir.substr(0, pos) + "]";
        }
        else
            out = "[" + dir + "]";
    }
    return out;
}

bool FileDatabase::isWildcard(const std::string& str)
{
    return str.find('*') != std::string::npos;
}

int FileDatabase::getUIC_hi(const std::string& in)
{
    int uic = 0;
    if (in.length() == 6)
    {
        bool allnum = true;
        //for (auto cit = in.cbegin(); allnum && (cit != in.cend()); ++cit)
        for (const auto& cit : in)
            allnum = (cit >= '0') && (cit <= '7');
        if (allnum)
        {
            uic = strtol(in.substr(0, 3).c_str(), nullptr, 8);
        }
    }
    return uic;
}

int FileDatabase::getUIC_lo(const std::string& in)
{
    int uic = 0;
    if (in.length() == 6)
    {
        bool allnum = true;
        //for (auto cit = in.cbegin(); allnum && (cit != in.cend()); ++cit)
        for (const auto& cit : in)
            allnum = (cit >= '0') && (cit <= '7');
        if (allnum)
        {
            uic = strtol(in.substr(3).c_str(), nullptr, 8);
        }
    }
    return uic;
}

std::string FileDatabase::makeKey(const std::string& dir)
{
    std::string key;
    //for (auto cit = dir.cbegin(); cit != dir.cend(); ++cit) {
    for (const auto& cit : dir) {
        if ((cit != '[') && (cit != ']') && (cit != ','))
            key += cit;
    }
    return key;
}

void FileDatabase::FindMatchingFiles(const char* dir, const char* filename, DirList_t& dirList, std::function<void(int, Files11Base& obj)> fetch)
{
    FindDirectory(dir, dirList);
    for (auto& dir : dirList)
    {
        Files11Record drec;
        if (Get(dir.fnumber, drec))
        {
            int vbn = 1;
            int eof_block = drec.GetFileFCS().GetEOFBlock();
			int eof_byte = drec.GetFileFCS().GetFirstFreeByte();
            Files11Base file;
            int ext_file_number = dir.fnumber;
            do {
                fetch(FileNumberToLBN(ext_file_number), file);
                F11_MapArea_t* pMap = file.GetMapArea();
                Files11Base::BlockList_t blklist;
                ext_file_number = Files11Base::GetBlockPointers(pMap, blklist);
                for (auto& block : blklist)
                {
                    for (auto lbn = block.lbn_start; (lbn <= block.lbn_end) && (vbn <= eof_block); ++lbn, ++vbn)
                    {
                        Files11Base file;
                        fetch(lbn, file);
                        DirectoryRecord_t* pDir = file.GetDirectoryRecord();
                        int nbrecs = (vbn == eof_block) ? eof_byte / sizeof(DirectoryRecord_t) : F11_BLOCK_SIZE / sizeof(DirectoryRecord_t);
                        for (int i = 0; i < nbrecs; ++i)
                        {
                            if (pDir[i].fileNumber != 0)
                            {
                                Files11Record frec;
                                if (Get(pDir[i].fileNumber, frec, filename)) {
                                    if (pDir[i].version == 0)
                                        printf("version 0\n");
                                    dir.fileList.push_back(DirFileRecord_t(pDir[i].fileNumber, pDir[i].fileSeq, pDir[i].version, frec.GetFileName(), frec.GetFileExt()));
                                }
                            }
                        }
                    }
                }
            } while ((ext_file_number > 0) && (vbn <= eof_block));
        }
    }
}