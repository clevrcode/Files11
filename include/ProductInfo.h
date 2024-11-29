#pragma once

#include "string"

class ProductInfo
{
public:
	ProductInfo();
	const char* GetProductName(void) { return strProductName.c_str(); };
	const char* GetProductVersion(void) { return strProductVersion.c_str(); };

private:
	std::string strProductName;
	std::string strProductVersion;
};

