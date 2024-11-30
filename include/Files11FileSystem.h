#pragma once
#include <iostream>
#include <fstream>

class Files11FileSystem
{
public:
	Files11FileSystem();
	~Files11FileSystem();

	bool Open(const char* diskFile);
	int  GetDiskSize(void) { return static_cast<int>(iDiskSize); };
	const char* GetErrorMessage(void) { return strErrorMsg.c_str(); };

private:

	std::string    strErrorMsg;
	std::streampos iDiskSize; // size of disk in bytes
	
	// low level services
	bool readBlock(int lbn, char *pBlk, std::ifstream &istrm);



};

