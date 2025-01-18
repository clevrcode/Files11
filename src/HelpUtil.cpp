#include <algorithm>
#include <iostream>
#include "HelpUtil.h"

HelpUtil::HelpTopics_t HelpUtil::HelpTopics[] = {
    { "HELP"  , "Print lis of available commands" },
    { "PWD"   , "Print the current working directory" },
    { "CD"    , "Change the current working directory" },
    { "DIR"   , "Print the content of the specified directory(ies) or the current working directory" },
    { "DMPLBN", "Dump the specified LBN" },
    { "DMPHDR", "Dump the header content for the specified file number" },
    { "CAT"   , "List the content of the specified file" },
    { "TYPE"  , "List the content of the specified file" },
    { "TIME"  , "Print the current system time" },
    { "FREE"  , "Print information about disk usage" },
    { "IMPORT", "Import a file from the host to the specified directory, or the current directory" },
    { "EXPORT", "Export the specified files to the host directory" },
    { "DEL"   , "Delete the specified file" },
    { "RM"    , "Delete the specified file" },
    { "LSFULL", "Print a full list of files" },
    { nullptr , "?????" }
};

HelpUtil::HelpUtil(void)
{
    // Initialize help
    for (int i = 0; HelpTopics[i].topic != nullptr; ++i)
        commands.push_back(HelpTopics[i].topic);
    std::sort(commands.begin(), commands.end());
}

void HelpUtil::PrintHelp(std::vector<std::string>& args)
{
    if (args.size() == 1)
    {
        std::cout << "\nHelp is available for the following commands :\n\n";
        std::cout << std::left;
        int item = 0;
        for (auto cmd : commands)
        {
            std::cout.width(16);
            std::cout << std::left << cmd;
            if ((++item % 5) == 0)
                std::cout << std::endl;
        }
        std::cout << std::endl << std::endl;
        std::cout << "For information on a command, type HELP commandname." << std::endl << std::endl;
    }
    else if (args.size() == 2)
    {
        bool found = false;
        std::string topic(args[1]);
        for (int i = 0; (HelpTopics[i].topic != nullptr) && !found; ++i)
        {
            if (found = (topic == HelpTopics[i].topic)) {
                std::cout << std::endl << HelpTopics[i].details << std::endl;
            }
        }
        if (!found) {
            std::cout << "Unknown topic\n";
        }
        std::cout << std::endl;
    }
}
