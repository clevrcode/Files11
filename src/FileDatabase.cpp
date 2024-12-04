#include "FileDatabase.h"

FileDatabase::FileDatabase(void)
{
}

bool FileDatabase::Add(int nb, const Files11Record &frec)
{
    auto it = m_Database.find(nb);
    if (it != m_Database.end()) {
        return false;;
    }
    else
    {
        // Add a new entry for this file (key: file number)
        m_Database[nb] = frec;
    }
    return true;
}

bool FileDatabase::Get(int nb, Files11Record& frec)
{
    auto cit = m_Database.find(nb);
    if (cit != m_Database.end()) {
        frec = cit->second;
        return true;
    }
    return false;
}

bool FileDatabase::Get(int nb, Files11Record& frec, int version, const char *filter)
{
    auto cit = m_Database.find(nb);
    if (cit != m_Database.end()) {
        if (Filter(cit->second, filter, version))
        {
            frec = cit->second;
            return true;
        }
    }
    return false;
}

bool FileDatabase::Filter(const Files11Record& rec, const char* name, int version) const
{
    if (name)
    {
        std::string fname(name);
        // split file name
        auto pos = fname.find('.');
        if (pos == std::string::npos)
        {
            // no extension
            return (fname == rec.GetFileName()) && (rec.GetFileExt() == "");
        }
        std::string ext(fname.substr(pos + 1));
        fname = fname.substr(0, pos);
        pos = fname.find(";");
        if (pos == std::string::npos)
        {
            // no version specified
            return (fname == rec.GetFileName()) && (ext == rec.GetFileExt());
        }
        std::string strVersion(ext.substr(pos + 1));
        if (strVersion == "*") {
            // any version
            return (fname == rec.GetFileName()) && (ext == rec.GetFileExt());
        }
        int nVersion = strtol(strVersion.c_str(), NULL, 10);
        return (fname == rec.GetFileName()) && (ext == rec.GetFileExt()) && (nVersion == version);
    }
    return true;
}

