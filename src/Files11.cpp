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
#include "HelpUtil.h"

const char DEL        = 0x08;
const char* PROMPT    = ">";

std::vector<std::string> commandQueue;
static void RunCLI(Files11FileSystem &fs);
static void ProcessCommand(std::string& command, Files11FileSystem& fs);
static void GetFileList(std::string search_path, std::vector<std::string>& list);
HelpUtil help;

int main(int argc, char *argv[])
{
    ProductInfo product;
    product.PrintGreetings();  
    if (argc != 2)
    {
        product.PrintUsage();
        return 0;
    }

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

static void del(size_t n=1)
{
    for (auto i = 0; i < n; i++) {
        _putch(DEL);
        _putch(' ');
        _putch(DEL);
    }
}

//-----------------------------------------
// Put a string on the command line, no CR
static void putstring(const char* str)
{
    for (auto i = 0; str[i] != '\0'; ++i)
        _putch(str[i]);
}

static std::string stripWhitespaces(std::string str)
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
static size_t parseCommand(std::string& command, Words_t& words)
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

static void RunCLI(Files11FileSystem &fs)
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


//
// HELP, PWD, CD, DIR, DMPLBN, DMPHDR, CAT, TYPE, TIME, FREE, IMPORT, EXPORT
//
static void ProcessCommand(std::string &command, Files11FileSystem& fs)
{
    Files11FileSystem::Args_t words;
    size_t nbWords = 0;
    if ((nbWords = parseCommand(command, words)) > 0)
    {
        if (words[0] == "HELP")
        {
            std::vector<std::string> args(words);
            help.PrintHelp(args);
        }
        else if (words[0] == "PWD")
        {
            std::cout << fs.GetCurrentWorkingDirectory() << std::endl;
        }
        else if (words[0] == "CD")
        {
            if (nbWords == 2)
                fs.ChangeWorkingDirectory(words[1].c_str());
            else
                std::cout << "ERROR -- missing argument\n";
        }
        else if (words[0] == "VFY")
        {
            fs.VerifyFileSystem(words);
        }
        else if (words[0] == "DIR")
        {
            fs.ListDirs(words);
        }
        else if (words[0] == "TIME")
        {
            std::cout << fs.GetCurrentSystemTime();
        }
        else if (words[0] == "FREE")
        {
            fs.PrintFreeBlocks();
        }
        else if ((words[0].substr(0, 3) == "DEL") || (words[0] == "RM"))
        {
            if (nbWords >= 2) {
                fs.DeleteFile(words);
            }
            else
                std::cout << "ERROR -- missing argument\n";
        }
        else if (words[0] == "DMPLBN")
        {
            if (nbWords >= 2)
                fs.DumpLBN(words);
            else
                std::cout << "ERROR -- missing argument\n";
        }
        else if (words[0] == "DMPHDR")
        {
            if (nbWords >= 2) {
                fs.DumpHeader(words);
            }
            else
                std::cout << "ERROR -- missing argument\n";
        }
        else if ((words[0] == "CAT") || (words[0] == "TYPE"))
        {
            if (nbWords == 2) {
                std::string dir, file;
                Files11FileSystem::SplitFilePath(words[1], dir, file);
                //fs.ListDirs(Files11FileSystem::TYPE, dir.c_str(), file.c_str());
            }
            else
                std::cout << "ERROR -- missing argument\n";
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
                Files11FileSystem::SplitFilePath(words[1], dir, file);
                if (dir.empty())
                    dir = fs.GetCurrentWorkingDirectory();
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

static void GetFileList(std::string search_path, std::vector<std::string>& list)
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

