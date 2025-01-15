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
        // Enter CLI interactive mode, user enter Q[uit] to exit
        RunCLI(F11_fs);
    }
    else
        std::cout << "Failed to open " << argv[1] << std::endl;
    return 0;
}

// ---------------------------------------------------
// Delete n character at the end of the command line

static void del(size_t n=1)
{
    for (int i = 0; i < n; i++) {
        // Move cursor back, then delete character at cursor
        std::cout << "\x1b[D" << "\x1b[P";
    }
}

//-----------------------------------------
// Put a string on the command line, no CR
static void putstring(const char* str)
{
    std::cout << str;
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
    const char ESC = 0x1b;
    const char* CURBACK = "\x1b[D";
    const char* CURFORW = "\x1b[C";

    const char* PROMPT = ">";

    std::vector<std::string> commandQueue;
    std::string command;
    size_t currCommand = 0;

    DWORD prevOutMode, prevInMode;

    // Set output mode to handle virtual terminal sequences
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE)
    {
        std::cerr << "Failed to get output console handle, error [" << GetLastError() <<"]\n";
        return;
    }
    HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
    if (hIn == INVALID_HANDLE_VALUE)
    {
        std::cerr << "Failed to get input console handle, error [" << GetLastError() << "]\n";
        return;
    }

    DWORD dwMode = 0;
    if (!GetConsoleMode(hOut, &dwMode))
    {
        std::cerr << "Failed to get output console mode, error [" << GetLastError() << "]\n";
        return;
    }
    prevOutMode = dwMode;
    dwMode |= (ENABLE_VIRTUAL_TERMINAL_PROCESSING | DISABLE_NEWLINE_AUTO_RETURN);
    if (!SetConsoleMode(hOut, dwMode))
    {
        std::cerr << "Failed to enable output virtual terminal console mode, error [" << GetLastError() << "]\n";
        return;
    }

    if (!GetConsoleMode(hIn, &dwMode))
    {
        std::cerr << "Failed to get input console mode, error [" << GetLastError() << "]\n";
        return;
    }
    prevInMode = dwMode;
    dwMode |= ENABLE_VIRTUAL_TERMINAL_INPUT;
    dwMode &= ~(ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT);

    if (!SetConsoleMode(hIn, dwMode))
    {
        std::cerr << "Failed to enable input virtual terminal console mode, error [" << GetLastError() << "]\n";
        return;
    }

    fs.PrintVolumeInfo();
    std::cout << PROMPT;

    int cmd_ptr = 0;
    for (;;)
    {
        size_t nbCommands = commandQueue.size();
        char buffer[16];
        DWORD nbChar;
        memset(buffer, 0, sizeof(buffer));

        ReadConsole(hIn, &buffer, sizeof(buffer), &nbChar, NULL);
        int key = buffer[0];
        if (key != '\r') {
            if (key == 0x1b) 
            {
                std::string escseq(buffer);
                if (escseq == "\x1b[A") // up-arrow
                {
                    // erase current line
                    if (currCommand > 0) {
                        del(command.length());
                        command = commandQueue[--currCommand];
                        putstring(command.c_str());
                        cmd_ptr = (int)command.length();
                    }
                }
                else if (escseq == "\x1b[B") // down-arrow
                {
                    if ((commandQueue.size() > 0) && (currCommand < (commandQueue.size() - 1))) {
                        del(command.length());
                        command = commandQueue[++currCommand];
                        putstring(command.c_str());
                        cmd_ptr = (int)command.length();
                    }
                }
                else if (escseq == "\x1b[D") // Left arrow
                {
                    if (cmd_ptr > 0) {
                        cmd_ptr--;
                        std::cout << CURBACK;
                    }
                }
                else if (escseq == "\x1b[C") // right arrow
                {
                    if (cmd_ptr < command.length()) {
                        cmd_ptr++;
                        std::cout << CURFORW;
                    }
                }
                else if (escseq == "\x1b[H") // Home
                {
                    std::cout << "\x1b[" << cmd_ptr << "D";
                    cmd_ptr = 0;
                }                
                else if (escseq == "\x1b[F") // End
                {
                    size_t cursor_move = command.length() - cmd_ptr;
                    cmd_ptr = (int)command.length();
                    if (cursor_move > 0) 
                        std::cout << "\x1b[" << cursor_move << "C";
                }
                else if (escseq == "\x1b[3~") //Delete
                {
                    command.erase(command.begin() + cmd_ptr);
                    std::cout << "\x1b[P";
                }
            }
            else if (key == 0x7f) // Backspace
            {
                if ((cmd_ptr > 0) && (command.length() > 0)) {
                    del();
                    if (cmd_ptr >= command.length()) {
                        command.pop_back();
                        cmd_ptr--;
                    }
                    else
                    {
                        cmd_ptr--;
                        command.erase(command.begin() + cmd_ptr);
                    }
                }
            }
            else
            {
                key = toupper(key);
                if (cmd_ptr >= command.length()) {
                    command += (char)key;
                    cmd_ptr++;
                    std::cout << (char)key;
                }
                else {
                    int p = cmd_ptr;
                    command.insert(command.begin() + cmd_ptr, (char)key);
                    cmd_ptr++;
                    std::cout << "\x1b[@" << (char)key;
                }
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
                cmd_ptr = 0;
            }
            std::cout << PROMPT;
        }
    }

    // Put back to original console mode
    SetConsoleMode(hIn, prevInMode);
    SetConsoleMode(hOut, prevOutMode);
    CloseHandle(hIn);
    CloseHandle(hOut);
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
                Files11FileSystem::print_error("ERROR -- missing argument");
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
                Files11FileSystem::print_error("ERROR -- missing argument");
        }
        else if (words[0] == "DMPLBN")
        {
            if (nbWords >= 2)
                fs.DumpLBN(words);
            else
                Files11FileSystem::print_error("ERROR -- missing argument");
        }
        else if (words[0] == "DMPHDR")
        {
            if (nbWords >= 2) {
                fs.DumpHeader(words);
            }
            else
                Files11FileSystem::print_error("ERROR -- missing argument");
        }
        else if ((words[0] == "CAT") || (words[0] == "TYPE"))
        {
            if (nbWords >= 2) {
                fs.ListFiles(words);
            }
            else
                Files11FileSystem::print_error("ERROR -- missing argument");
        }
        else if (words[0] == "LSFULL")
        {
            fs.FullList(words);
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
                    Files11FileSystem::print_error("ERROR -- File not found");
                }
            }
            else
                Files11FileSystem::print_error("ERROR -- missing argument");
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
            Files11FileSystem::print_error("ERROR -- Unknown command");
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

