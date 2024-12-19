#include "FileDatabase.h"

bool FileDatabase::Add(int nb, const Files11Record &frec)
{
    // Add a new entry for this file (key: file number)
    m_Database[nb] = frec;
    return true;
}

bool FileDatabase::Delete(int nb)
{
    m_Database.erase(nb);
    return true;
}

int  FileDatabase::FindFirstFreeFile(void)
{
    int fileNumber = -1;
    for (int fnb = F11_CORIMG_SYS + 1; (fnb < m_MaxFileNumber) && (fileNumber <= 0); ++fnb) {
        auto cit = m_Database.find(fnb);
        if (cit == m_Database.end()) {
            // Mark it as used with an empty record
            Files11Record frec;
            m_Database[fnb] = frec;
            fileNumber = fnb;
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

bool FileDatabase::Get(int nb, Files11Record& frec, int version, const char *fname)
{
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
    if (fullname)
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

