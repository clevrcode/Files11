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

void RunCLI(Files11FileSystem &fs)
{
    std::string command;
    std::cout << PROMPT;
    size_t currCommand = 0;
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
            if (command.length() > 0)
            {
                // Quit
                if (command.front() == 'Q')
                    break;

                commandQueue.push_back(command);
                currCommand = commandQueue.size();

                std::cout << "\nUnknown command: " << command;
                command.clear();
            }
            std::cout << std::endl << PROMPT;
        }
    }
}
