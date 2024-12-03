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
    Files11FileSystem F11_fs;
    if (F11_fs.Open(argv[1]))
    {
        std::cout << "----------------------------------------------------\n";
        F11_fs.PrintVolumeInfo();
        std::cout << "----------------------------------------------------\n";

        //F11_fs.ListFiles("*.*");
        //F11_fs.ListDirs("IPLISP");
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
    auto pos = str.find_first_not_of(' ');
    if (pos != std::string::npos)
        return str.substr(pos);
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
                        if (nbWords == 2)
                            std::cout << "change working directory to " << words[1] << std::endl;
                        else
                            std::cout << "CD error: missing argument\n";
                    }
                    else if (words[0] == "DIR")
                    {
                        if (nbWords == 2)
                            fs.ListDirs(words[1].c_str());
                        else
                            fs.ListDirs(NULL);
                    }
                    else
                    {
                        std::cout << "Unknown command: " << words[0] << std::endl;
                    }
                }
                command.clear();
            }
            std::cout << PROMPT;

        }
    }
}
