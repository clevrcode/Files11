#pragma once
#include <vector>
#include <string>

class HelpUtil
{
public:
    HelpUtil(void);

    void PrintHelp(std::vector<std::string> &args);

    // static topic detailing functions
    static void Help_HELP(void);
    static void Help_PWD(void);
    static void Help_CD(void);
    static void Help_DIR(void);
    static void Help_DMPLBN(void);
    static void Help_DMPHDR(void);
    static void Help_CAT(void);
    static void Help_TYPE(void);
    static void Help_TIME(void);
    static void Help_FREE(void);
    static void Help_IMPORT(void);
    static void Help_EXPORT(void);
    static void Help_DEL(void);
    static void Help_RM(void);
    static void Help_LSFULL(void);
    static void Help_PURGE(void);

    typedef struct HelpTopics_ {
        const char* topic;
        void (*detail)(void);
    } HelpTopics_t;

private:
    static HelpTopics_t HelpTopics[];
    std::vector<std::string> commands;
};

