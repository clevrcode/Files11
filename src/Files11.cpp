// Files11.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <Windows.h>
#include <conio.h>
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include "ProductInfo.h"
#include <Files11FileSystem.h>

const char DEL = 0x08;
const char* PROMPT = ">";
std::vector<std::string> commandQueue;
void RunCLI(Files11FileSystem &fs);
void ProcessCommand(std::string& command, Files11FileSystem& fs);
void GetFileList(std::string search_path, std::vector<std::string>& list);
void PrintHelp(void);
void PrintHelp(std::string &topic);

struct sHelpTopics {
    const char * topic;
    const char* details;
} HelpTopics[] = {
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
    { "IMPORT", "Import a file to the specified directory, or the current directory" },
    { "EXPORT", "Export the specified file9s) to the native directory" },
    { nullptr , "?????" }
};

std::vector<std::string> commands;

int main(int argc, char *argv[])
{
    ProductInfo product;
    product.PrintGreetings();  
    if (argc != 2)
    {
        product.PrintUsage();
        return 0;
    }

    // Initialize help
    for (int i = 0; HelpTopics[i].topic != nullptr; ++i)
        commands.push_back(HelpTopics[i].topic);
    std::sort(commands.begin(), commands.end());

    std::cout << "Opening disk file " << argv[1] << std::endl;

    // Files-11 File System object
    Files11FileSystem F11_fs;
    if (F11_fs.Open(argv[1]))
    {
        std::cout << "----------------------------------------------------\n";
        F11_fs.PrintVolumeInfo();
        std::cout << "----------------------------------------------------\n";

        // Enter CLI interactive mode, user enter Q[uit] to exit
        RunCLI(F11_fs);
    }
    else
        std::cout << "Failed to open " << argv[1] << std::endl;
    return 0;
}

// ---------------------------------------------------
// Delete n character at teh end of the command line

void del(size_t n=1)
{
    for (auto i = 0; i < n; i++) {
        _putch(DEL);
        _putch(' ');
        _putch(DEL);
    }
}

//-----------------------------------------
// Put a string on the command line, no CR
void putstring(const char* str)
{
    for (auto i = 0; str[i] != '\0'; ++i)
        _putch(str[i]);
}

std::string stripWhitespaces(std::string str)
{
    if (str.length() > 0)
    {
        std::string out(str);
        // strip front whitespaces
        auto pos = str.find_first_not_of(' ');
        if (pos != std::string::npos)
            out = str.substr(pos);
        // strip back whitespaces
        while (out.back() == ' ')
            out.pop_back();
        return out;
    }
    return str;
}

typedef std::vector<std::string> Words_t;
size_t parseCommand(std::string& command, Words_t& words)
{
    command = stripWhitespaces(command);
    do {
        auto pos = command.find_first_of(' ');
        if (pos != std::string::npos)
        {
            words.push_back(command.substr(0, pos));
            command = stripWhitespaces(command.substr(pos));
        }
        else
        {
            words.push_back(command);
            command.clear();
        }
    } while (command.length() > 0);
    return words.size();
}

void RunCLI(Files11FileSystem &fs)
{
    std::string command;
    size_t currCommand = 0;
    std::cout << PROMPT;
    //HANDLE hConsole = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);
    //SetConsoleActiveScreenBuffer(hConsole);
    //DWORD dwBytesWritten = 0;
    //COORD pos = { 0, 0 };
    //WriteConsoleOutputCharacter(hConsole, PROMPT, (DWORD)strlen(PROMPT), pos, &dwBytesWritten);

    for (;;)
    {
        size_t nbCommands = commandQueue.size();
        int key = _getch();
        if (key != '\r') {
            if (key == 0xe0) {
                key = _getch();
                if (key == 0x48) // up-arrow
                {
                    // erase current line
                    if (currCommand > 0) {
                        del(command.length());
                        command = commandQueue[--currCommand];
                        putstring(command.c_str());
                    }
                }
                else if (key == 0x50) // down-arrow
                {
                    if (currCommand < (commandQueue.size() - 1)) {
                        del(command.length());
                        command = commandQueue[++currCommand];
                        putstring(command.c_str());
                    }
                }
                else if (0x4b) // Left arrow
                {

                }
                else if (0x4d) // right arrow
                {

                }
            }
            else if (key == 0x08)
            {
                if (command.length() > 0) {
                    del();
                    command.pop_back();
                }
            }
            else
            {
                key = toupper(key);
                command += key;
                _putch(key);
            }
        }
        else
        {
            std::cout << std::endl;
            if (command.length() > 0)
            {
                // Quit
                if (command.front() == 'Q')
                    break;

                commandQueue.push_back(command);
                currCommand = commandQueue.size();
                ProcessCommand(command, fs);
                command.clear();
            }
            std::cout << PROMPT;

        }
    }
    //CloseHandle(hConsole);
}

void SplitFilePath(const std::string &path, std::string &dir, std::string &file)
{
    // split dir and file
    auto pos = path.find(']');
    if (pos != std::string::npos)
    {
        dir = path.substr(0, pos + 1);
        file = ((pos + 1) < path.length()) ? path.substr(pos + 1) : "*.*;*";
    }
    else
    {
        dir = "";
        file = path;
    }
}

//
// HELP, PWD, CD, DIR, DMPLBN, DMPHDR, CAT, TYPE, TIME, FREE, IMPORT, EXPORT
//
void ProcessCommand(std::string &command, Files11FileSystem& fs)
{
    Files11FileSystem::Args_t words;
    size_t nbWords = 0;
    if ((nbWords = parseCommand(command, words)) > 0)
    {
        if (words[0] == "HELP")
        {
            if (nbWords == 2)
                PrintHelp(words[1]);
            else
                PrintHelp();
        }
        else if (words[0] == "PWD")
        {
            std::cout << fs.GetCurrentWorkingDirectory() << std::endl;
        }
        else if (words[0] == "VFY")
        {
            fs.VerifyFileSystem(words);
        }
        else if (words[0] == "CD")
        {
            if (nbWords == 2) {
                fs.ChangeWorkingDirectory(words[1].c_str());
            }
            else
                std::cout << "ERROR -- missing argument\n";
        }
        else if (words[0] == "DIR")
        {
            if (nbWords == 2) {
                std::string dir, file;
                SplitFilePath(words[1], dir, file);
                fs.ListDirs(Files11FileSystem::LIST, dir.c_str(), file.c_str());
            }
            else
                fs.ListDirs(Files11FileSystem::LIST, nullptr, nullptr);
        }
        else if (words[0] == "DMPLBN")
        {
            if ((nbWords == 2)&&(words[1].length() > 1))
            {
                int base = 10;
                if (words[1][0] == '0')
                    base = 8;
                else if ((words[1][0] == 'x') || (words[1][0] == 'X'))
                    base = 16;
                int lbn = strtol(words[1].c_str(), NULL, base);
                fs.DumpLBN(lbn);
            }
            else
                std::cout << "ERROR -- missing argument\n";
        }
        else if (words[0] == "DMPHDR")
        {
            if ((nbWords == 2) && (words[1].length() > 1))
            {
                int base = 10;
                if (words[1][0] == '0')
                    base = 8;
                else if ((words[1][0] == 'x') || (words[1][0] == 'X'))
                    base = 16;
                int fnb = strtol(words[1].c_str(), NULL, base);
                fs.DumpHeader(fnb);
            }
        }
        else if ((words[0] == "CAT") || (words[0] == "TYPE"))
        {
            if (nbWords == 2) {
                std::string dir, file;
                SplitFilePath(words[1], dir, file);
                fs.ListDirs(Files11FileSystem::TYPE, dir.c_str(), file.c_str());
            }
            else
                std::cout << "ERROR -- missing argument\n";
        }
        else if (words[0] == "TIME")
        {
            std::cout << fs.GetCurrentSystemTime();
        }
        else if (words[0] == "FREE")
        {
            fs.PrintFreeBlocks();
        }
        else if ((words[0].substr(0,3) == "IMP") || (words[0] == "UP"))
        {
            std::string dir;
            if (nbWords == 2) {
                // If destination is not specified, set it to current working dir
                words.push_back(fs.GetCurrentWorkingDirectory());
            }
            if (words.size() == 3) {
                dir = words[2];
                std::vector<std::string> list;
                GetFileList(words[1], list);
                if (list.size() > 0)
                {
                    for (auto localfile : list)
                    {
                        std::string pdpfile(localfile);
                        auto pos = localfile.rfind("/");
                        if (pos != std::string::npos)
                            pdpfile = localfile.substr(pos + 1);
                        fs.AddFile(localfile.c_str(), dir.c_str(), pdpfile.c_str());
                    }
                }
                else {
                    std::cout << "ERROR -- File not found\n";
                }
            }
            else {
                std::cout << "ERROR -- Missing argument\n";
            }
        }
        else if ((words[0].substr(0, 3) == "EXP") || (words[0] == "DOWN"))
        {
            std::string dir;
            if (nbWords >= 2) {
                std::string dir, file, output;
                SplitFilePath(words[1], dir, file);
                if (nbWords == 2)
                    words.push_back(".");
                fs.ExportFiles(dir.c_str(), file.c_str(), words[2].c_str());
            }
        }
        else
        {
            std::cout << "Unknown command: " << words[0] << std::endl;
        }
    }
}

void GetFileList(std::string search_path, std::vector<std::string>& list)
{
    list.clear();
    std::string dir;
    auto pos = search_path.rfind("/");
    if (pos != std::string::npos)
        dir = search_path.substr(0, pos+1);

    WIN32_FIND_DATA fd;
    HANDLE hFind = ::FindFirstFile(search_path.c_str(), &fd);
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            // read all (real) files in current folder
            if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                list.push_back(dir + fd.cFileName);
            }
        } while (::FindNextFile(hFind, &fd));
        ::FindClose(hFind);
    }
}

//
// For help in logging into the system, type HELP HELLO or HELP LOGIN.
// You'll need a user-ID and password to log in.  Ask your system manager.
//
// Help is available on the following MCR commands :
//
// ABORT         ACD             ACS             ACTIVE          ALL
// ALTER         ASN             ATL             BLK             BOOT
// BRK           BRO             BYE             CANCEL          CBD
// CLI           CLQ             DCL             DEA             DEBUG
// DEV           DFL             DMO             FIX             FLAG
// HELLO         HELP            HOME            INI             INS
// LOAD          LOGIN           LUN             MOUNT           OPEN
// PAR           REA             RED             REMOVE          RESUME
// RUN           SAVE            SET             SSM             SWR
// TAL           TAS             TIME            UFD             UNB
// UNFIX         UNLOAD          UNSTOP          Dates
// 
// For information on help for utilities and other system features, type
// HELP MORE.For information on a command, type HELP commandname.
//

void PrintHelp(void)
{

    std::cout << "\nHelp is available on the following MCR commands :\n\n";
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

void PrintHelp(std::string& topic)
{
    bool found = false;
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
