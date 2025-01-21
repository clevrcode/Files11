#include <algorithm>
#include <iostream>
#include "HelpUtil.h"

HelpUtil::HelpTopics_t HelpUtil::HelpTopics[] = {
    { "HELP"  , Help_HELP },
    { "PWD"   , Help_PWD },
    { "CD"    , Help_CD },
    { "DIR"   , Help_DIR },
    { "DMPLBN", Help_DMPLBN },
    { "DMPHDR", Help_DMPHDR },
    { "CAT"   , Help_CAT },
    { "TYPE"  , Help_TYPE },
    { "FREE"  , Help_FREE },
    { "IMPORT", Help_IMPORT },
    { "EXPORT", Help_EXPORT },
    { "DEL"   , Help_DEL },
    { "RM"    , Help_RM },
    { "LSFULL", Help_LSFULL },
    { "PURGE" , Help_PURGE },
    { nullptr , nullptr }
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
        std::cout << "For information on a command, type HELP command." << std::endl << std::endl;
    }
    else if (args.size() == 2)
    {
        bool found = false;
        std::string topic(args[1]);
        for (int i = 0; (HelpTopics[i].topic != nullptr) && !found; ++i)
        {
            if (found = (topic == HelpTopics[i].topic)) {
                HelpTopics[i].detail();
            }
        }
        if (!found) {
            std::cout << "Unknown topic\n";
        }
        std::cout << std::endl;
    }
}

void HelpUtil::Help_HELP(void)
{
    std::cout << "\n\nThe HELP command displays a brief description of a command usage.\n\n";
}

void HelpUtil::Help_PWD(void)
{
    std::cout << "\n";
    std::cout << "\n   >PWD\n";
    std::cout << "\n   Displays the current working directory.\n\n";
}

void HelpUtil::Help_CD(void)
{
    std::cout << "\n";
    std::cout << "\n   >CD [nnn, mmm]";
    std::cout << "\n   >CD [dirname]\n";
    std::cout << "\n   Used to change the current working directory.\n\n";
}

void HelpUtil::Help_DIR(void)
{
    std::cout << "\n";
    std::cout << "\n   >DIR <UFD>[file-spec]\n";
    std::cout << "\n   List the content of the specified directory/directories";
    std::cout << "\n   The directory and file specification can include '*' wildcard character.\n";
    std::cout << "\n   Example: DIR [*]         : List content of all directories.";
    std::cout << "\n            DIR [2,*]       : List content of all directories in group 2.";
    std::cout << "\n            DIR [2,54]      : List all files in directory [2,54].";
    std::cout << "\n            DIR [*]FILE.CMD : List all highest version instances of file 'FILE.CMD' in any directory.";
    std::cout << "\n            DIR FILE.CMD;*  : List all version(s) of 'FILE.CMD' on the current working directory.";
    std::cout << "\n\n";
}

void HelpUtil::Help_DMPLBN(void)
{
    std::cout << "\n";
    std::cout << "\n   >DMPLBN <lbn>\n";
    std::cout << "\n   Dump the content of logical block number lbn.";
    std::cout << "\n   lbn must be a positive integer from 0 to number of blocks on the file system.\n";
    std::cout << "\n   lbn is octal if prefixed with a '0'.";
    std::cout << "\n   lbn is hexadecimal if prefixed with a 'x' or 'X' character.";
    std::cout << "\n   otherwise lbn is decimal.";
    std::cout << "\n\n";
}

void HelpUtil::Help_DMPHDR(void)
{
    std::cout << "\n";
    std::cout << "\n   >DMPHDR <file-spec>\n";
    std::cout << "\n   Dump the content of a file header (similar to PDP-11 'DMP /HD' command.)";
    std::cout << "\n\n";
}

void HelpUtil::Help_CAT(void)
{
    std::cout << "\n";
    std::cout << "\n   >CAT <file-spec>\n";
    std::cout << "\n   Display the content of a file.\n   If the file conatins binary, a binary dump will be displayed instead.";
    std::cout << "\n   This command is an alias to TYPE command.";
    std::cout << "\n\n";
}

void HelpUtil::Help_TYPE(void)
{
    std::cout << "\n";
    std::cout << "\n   >TYPE <file-spec>\n";
    std::cout << "\n   Display the content of a file.\n   If the file conatins binary, a binary dump will be displayed instead.";
    std::cout << "\n   This command is an alias to CAT command.";
    std::cout << "\n\n";
}

void HelpUtil::Help_FREE(void)
{
    std::cout << "\n";
    std::cout << "\n   >FREE\n";
    std::cout << "\n   Display the amount of available space on the disk, the largest contiguous space,";
    std::cout << "\n   the number of available headers and the number of headers used.";
    std::cout << "\n   This command is similar to PDP-11 'PIP /FR' command.";
    std::cout << "\n\n";
}

void HelpUtil::Help_IMPORT(void)
{
    std::cout << "\n";
    std::cout << "\n   >IMPORT <host-file-spec> [local-file-spec]";
    std::cout << "\n   >IMP    <host-file-spec> [local-file-spec]";
    std::cout << "\n   >UP     <host-file-spec> [local-file-spec]\n";
    std::cout << "\n   This command import/upload files from the host file system to the PDP-11 disk.";
    std::cout << "\n   The host file specifications can include wildcard and uses '/' directory delimiter.";
    std::cout << "\n   The local file specifier can specify a destination directory or default to the current directory.";
    std::cout << "\n   A specific local file can be specified if uploading a single file (useful if the host file doesn't";
    std::cout << "\n   match the 9.3 naming restrictions.)\n";
    std::cout << "\n   Example: >IMP data/*.txt [100,200]";
    std::cout << "\n            Will upload all .txt files from the host data directory to the [100,200] directory.\n";
    std::cout << "\n            >IMPORT longfilename.txt [100,200]LONG.TXT";
    std::cout << "\n            Will upload all .txt files from the host data directory to the [100,200] directory.";
    std::cout << "\n\n";
}

void HelpUtil::Help_EXPORT(void)
{
    std::cout << "\n   >EXPORT <local-file-spec> [host-directory]";
    std::cout << "\n   >EXP    <local-file-spec> [host-directory]";
    std::cout << "\n   >DOWN   <local-file-spec> [host-directory]\n";
    std::cout << "\n   Export or download PDP-11 files to the host file system.\n";
    std::cout << "\n   Example: >EXP [*]";
    std::cout << "\n            Export the whole PDP-11 volume to the host current working directory under a sub-directory named <volume-name>.\n";
    std::cout << "\n            >EXP [100,200]";
    std::cout << "\n            Export the content of the [100,200] directory to the host current working directory under a sub-directory named '<volume-name>/100200'.\n";
    std::cout << "\n            >EXP [3,54]*.CMD";
    std::cout << "\n            Export the latest version of all .CMD files under the [3,54] directory to the host current working directory under a sub-directory named '<volume-name>/003054'.\n";
    std::cout << "\n\n";
}

void HelpUtil::Help_DEL(void)
{
    std::cout << "\n";
    std::cout << "\n   >DEL [<UFD>]<file-spec>\n";
    std::cout << "\n   Delete file(s) from a directory. The file specification can have a directory.";
    std::cout << "\n   The file must specify a version number or '*' to delete all versions of the file(s).";
    std::cout << "\n   This command is an alias to the RM command.";
    std::cout << "\n\n";
}

void HelpUtil::Help_RM(void)
{
    std::cout << "\n";
    std::cout << "\n   >RM [<UFD>]<file-spec>\n";
    std::cout << "\n   Delete file(s) from a directory. The file specification can have a directory.";
    std::cout << "\n   The file must specify a version number or '*' to delete all versions of the file(s).";
    std::cout << "\n   This command is an alias to the DEL command.";
    std::cout << "\n\n";
}

void HelpUtil::Help_LSFULL(void)
{
    std::cout << "\n\n   TODO\n\n";
}

void HelpUtil::Help_PURGE(void)
{
    std::cout << "\n";
    std::cout << "\n   >PURGE\n";
    std::cout << "\n   Not implemented yet.";
    std::cout << "\n\n";
}
