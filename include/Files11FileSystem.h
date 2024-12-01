#pragma once
#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include "Files11Base.h"
#include "Files11HomeBlock.h"

class Files11FileSystem : public Files11Base
{
public:
	Files11FileSystem();
	~Files11FileSystem();

	bool Open(const char* diskFile);
	void PrintVolumeInfo(void);
	int  GetDiskSize(void);
	bool BuildFileDatabase(void);
	const char* GetErrorMessage(void) { return m_strErrorMsg.c_str(); };

private:
	bool             m_bValid;
	Files11HomeBlock m_HomeBlock;
	std::string      m_strErrorMsg;

};

