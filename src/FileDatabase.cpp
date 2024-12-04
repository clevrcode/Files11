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

bool FileDatabase::Get(int nb, Files11Record& frec, const char *filter)
{
    auto cit = m_Database.find(nb);
    if (cit != m_Database.end()) {
        if (Filter(frec, filter))
        {
            frec = cit->second;
            return true;
        }
    }
    return false;
}

bool FileDatabase::Filter(const Files11Record& rec, const char* name) const
{
    if (name)
    {
        // TODO
    }
    return true;
}

