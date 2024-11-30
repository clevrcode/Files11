#pragma once

#include "string"

class ProductInfo
{
public:
	ProductInfo();
	void PrintGreetings(void);
	void PrintUsage(void);
	const char* GetProductName(void) { return strProductName.c_str(); };
	const char* GetProductVersion(void) { return strProductVersion.c_str(); };

private:
	std::string strProductName;
	std::string strProductVersion;
};

