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

    Files11FileSystem F11_fs;
    F11_fs.Open(argv[1]);
    std::cout << "Opening disk file " << argv[1] << std::endl;
    std::cout << "Size of file is: " << F11_fs.GetDiskSize() << std::endl;
    return 0;
}

