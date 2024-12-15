#include <regex>
#include "DirDatabase.h"

bool DirDatabase::Add(const std::string& name, DirInfo_t &info)
{
    auto dit = m_Database.find(name);
    if (dit != m_Database.end())
        return false;

    m_Database[name] = info;
    return true;
}

bool DirDatabase::Exist(const char* dname) const
{
    
    std::string strDirName(makeKey(dname));
    auto pos = m_Database.find(strDirName);
    return pos != m_Database.end();
}

int DirDatabase::Find(const char *dname, DirList_t& dlist) const
{
    dlist.clear();

    // Check if dname contains wildcard
    std::string dirname(FormatDirectory(dname));
    if (isWildcard(dirname))
    {
        if ((dirname == "[*]")||(dirname == "[*,*]"))
        {
            // return all directories
            //for (auto cit = m_Database.cbegin(); cit != m_Database.cend(); ++cit)
            for (auto & cit : m_Database)
                dlist.push_back(cit.second);
        }
        else
        {
            std::smatch sm;
            std::regex_match(dirname, sm, std::regex("\\[\\*,([0-7]+)\\]"));
            if (sm.size() == 2)
            {
                int a = strtol(sm.str(1).c_str(), nullptr, 8);
                // return all directories
                for (auto & cit : m_Database)
                {
                    int b = getUIC_lo(cit.first);
                    if (a == b)
                        dlist.push_back(cit.second);
                }
            }
            else
            {
                std::regex_match(dirname, sm, std::regex("\\[([0-7]+),\\*\\]"));
                if (sm.size() == 2)
                {
                    int a = strtol(sm.str(1).c_str(), nullptr, 8);
                    // return all directories
                    //for (auto cit = m_Database.cbegin(); cit != m_Database.cend(); ++cit)
                    for (auto & cit : m_Database)
                    {
                        int b = getUIC_hi(cit.first);
                        if (a == b)
                            dlist.push_back(cit.second);
                    }
                }
                else
                {
                    // string wildcard
                }
            }
        }
    }
    else
    {
        std::string key(makeKey(dirname));
        auto pos = m_Database.find(key);
        if (pos != m_Database.end())
            dlist.push_back(pos->second);
    }
    // Return the number of directories found
    return (int) dlist.size();;
}

std::string DirDatabase:: FormatDirectory(const std::string& dir)
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

bool DirDatabase::isWildcard(const std::string& str)
{
    return str.find('*') != std::string::npos;
}

int DirDatabase::getUIC_hi(const std::string& in)
{
    int uic = 0;
    if (in.length() == 6)
    {
        bool allnum = true;
        //for (auto cit = in.cbegin(); allnum && (cit != in.cend()); ++cit)
        for (const auto & cit : in)
            allnum = (cit >= '0') && (cit <= '7');
        if (allnum)
        {
            uic = strtol(in.substr(0, 3).c_str(), nullptr, 8);
        }
    }
    return uic;
}

int DirDatabase::getUIC_lo(const std::string& in)
{
    int uic = 0;
    if (in.length() == 6)
    {
        bool allnum = true;
        //for (auto cit = in.cbegin(); allnum && (cit != in.cend()); ++cit)
        for (const auto & cit : in)
            allnum = (cit >= '0') && (cit <= '7');
        if (allnum)
        {
            uic = strtol(in.substr(3).c_str(), nullptr, 8);
        }
    }
    return uic;
}

std::string DirDatabase::makeKey(const std::string& dir)
{
    std::string key;
    //for (auto cit = dir.cbegin(); cit != dir.cend(); ++cit) {
    for (const auto & cit : dir) {
        if ((cit != '[') && (cit != ']') && (cit != ','))
            key += cit;
    }
    return key;
}

