#pragma once

#include <iostream>
#include <fstream>
#include <string>
#include <functional>
#include "Files11Base.h"

class Files11HomeBlock
{
public:
	Files11HomeBlock();
	bool Initialize(F11_HomeBlock_t* pHome);
	bool InitializeSCB(SCB_t* pSCB);

	void PrintInfo(void);
	const char* GetVolumeName(void) const { return strVolumeName.c_str(); };
	const int GetMaxFiles(void)     const { return iMaxFiles; };
	const int GetBitmapLBN(void)    const { return iIndexBitmapLBN; };
	const int GetIndexSize(void)    const { return iIndexBitmapSize;};
	const int GetIndexLBN(void)     const { return iIndexFileLBN; };
	const int GetBitmapSysLBN(void) const { return iBitmapSysLBN; };
	const int GetBadblkSysLBN(void) const { return iBadblkSysLBN; };
	const int Get000000SysLBN(void) const { return i000000SysLBN; };
	const int GetCorimgSysLBN(void) const { return iCorimgSysLBN; };
	const int GetVolumeOwner(void)  const { return iVolumeOwnerUIC; };
	const int GetDefaultFileProtection(void) const { return iDefaultFileProtection; };
	int  GetUsedHeaders(void) const { return iUsedHeaders; };
	int  GetFreeHeaders(void) const { return iMaxFiles - iUsedHeaders; };
	int  GetNumberOfBlocks(void) const { return iScbUnitSizeBlk; };
	const char* GetOwnerUIC(void) const { return strVolumeOwner.c_str(); };
	bool ValidateHomeBlock(F11_HomeBlock_t* pHome);
	void SetUsedHeaders(int nbUsedHeaders) { iUsedHeaders = nbUsedHeaders; };

private:
	int i000000SysLBN;
	int iIndexBitmapSize;
	int iBadblkSysLBN;
	int iBitmapSysLBN;
	int iCorimgSysLBN;
	int iCountHomeBlockRevision;
	int iDefaultFileExtend;
	int iDefaultFileProtection;
	int iDefaultWindowSize;
	int iDirectoryPreAccessLimit;
	int iIndexBitmapLBN;
	int iIndexFileLBN;
	int iMaxFiles;
	int iPackSerialNumber;
	int iScbNbBlocks;
	int iScbUnitSizeBlk;
	int iStorageBitmapClusterFactor;
	int iUsedHeaders;
	int iVolumeStructureLevel;
	int iVolumeOwnerUIC;
	int iVolumeProtectionCode;

	// DeviceType;
	std::string strVolumeName;
	std::string strLastRevision;
	std::string strVolumeCreationDate;
	std::string strVolumeOwner;
	std::string strFormatType;

	const int bitCount[16] = { 0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4 };
};
