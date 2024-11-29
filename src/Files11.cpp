// Files11.cpp : This file contains the 'main' function. Program execution begins and ends there.
//


#include <Windows.h>
#include <iostream>
#include "ProductInfo.h"

void PrintUsage(void)
{
    std::cout << "usage: >files11 <disk-file>\n";
    std::cout << "        <disk-file> is a valid Files-11 disk image file\n";
}

int main(int argc, char *argv[])
{
    ProductInfo product;
    std::cout << product.GetProductName() << " -- Version " << product.GetProductVersion() << std::endl;
    
    if (argc != 2)
    {
        PrintUsage();
        return 0;
    }




    return 0;
}

