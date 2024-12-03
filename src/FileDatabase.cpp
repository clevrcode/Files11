#include "FileDatabase.h"

FileDatabase::FileDatabase(void)
{
}

bool FileDatabase::Add(int nb, Files11Record &frec)
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


