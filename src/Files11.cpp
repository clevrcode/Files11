// Files11.cpp : This file contains the 'main' function. Program execution begins and ends there.
//


#include <Windows.h>
#include <iostream>
#include "ProductInfo.h"
#include <Files11FileSystem.h>

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
    }
    else
        std::cout << "Failed to open " << argv[1] << std::endl;
    return 0;
}

