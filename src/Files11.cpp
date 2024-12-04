// Files11.cpp : This file contains the 'main' function. Program execution begins and ends there.
//


#include <Windows.h>
#include <conio.h>
#include <iostream>
#include <string>
#include <vector>
#include "ProductInfo.h"
#include <Files11FileSystem.h>

const char DEL = 0x08;
const char* PROMPT = ">";
std::vector<std::string> commandQueue;
void RunCLI(Files11FileSystem &fs);
void ProcessCommand(std::string& command, Files11FileSystem& fs);


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
}

void ProcessCommand(std::string &command, Files11FileSystem& fs)
{
    Words_t words;
    size_t nbWords = 0;
    if ((nbWords = parseCommand(command, words)) > 0)
    {
        if (words[0] == "PWD")
        {
            std::cout << "Print current working directory\n";
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
                // split dir and file
                std::string dir, file;
                auto pos = words[1].find(']');
                if (pos != std::string::npos)
                {
                    dir = words[1].substr(0, pos + 1);
                    file = ((pos + 1) < words[1].length()) ? words[1].substr(pos + 1) : "*.*;*";
                }
                else
                {
                    dir = "";
                    file = words[1];
                }
                fs.ListDirs(dir.c_str(), file.c_str());
            }
            else
                fs.ListDirs(NULL, NULL);
        }
        else if ((words[0] == "CAT") || (words[0] == "TYPE"))
        {
            if (nbWords == 2)
                fs.TypeFile(words[1].c_str());
            else
                std::cout << "ERROR -- missing argument\n";
        }
        else
        {
            std::cout << "Unknown command: " << words[0] << std::endl;
        }
    }
}