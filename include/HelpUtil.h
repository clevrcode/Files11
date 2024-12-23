#pragma once
#include <vector>
#include <string>

class HelpUtil
{
public:
    HelpUtil(void);

    void PrintHelp(void);
    void PrintHelp(std::string& topic);

    typedef struct HelpTopics_ {
        const char* topic;
        const char* details;
    } HelpTopics_t;

private:
    static HelpTopics_t HelpTopics[];
    std::vector<std::string> commands;
};

