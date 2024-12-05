#pragma once

#include <iostream>
#include <fstream>
#include <string>
#include "Files11Base.h"

class Files11HomeBlock : public Files11Base
{
public:
	Files11HomeBlock();
	bool Initialize(const char *diskName);
	bool Initialize(std::ifstream& istrm);
	void PrintInfo(void);
	const int  GetMaxFiles(void) const { return iMaxFiles; };
	const int  GetBitmapLBN(void) const { return iIndexBitmapLBN; };
	const int  GetIndexLBN(void) const { return iIndexFileLBN; };
	int  GetDiskSize(void) { return bValid ? iDiskSize : 0;  };
	int  CountTotalFiles(std::ifstream& istrm);
	int  CountUsedHeaders(std::ifstream& istrm);
	int  GetUsedHeaders(void) const { return iUsedHeaders; };
	int  GetFreeHeaders(void) const { return iMaxFiles - iUsedHeaders; };
	int  GetNumberOfBlocks(void) const { return iScbUnitSizeBlk; };
	const char* GetOwnerUIC(void) const { return strVolumeOwner.c_str(); };
	bool ValidateHomeBlock(ODS1_HomeBlock_t* pHome);
	ODS1_HomeBlock_t* ReadHomeBlock(std::ifstream& istrm);

private:
	bool bValid;
	int iDiskSize; // size of disk in bytes
	int iIndexBitmapSize;
	int iScbNbBlocks;
	int iScbUnitSizeBlk;
	int iIndexBitmapLBN;
	int iIndexFileLBN;
	int iBitmapSysLBN;
	int iMaxFiles;
	int iTotalFiles;
	int iUsedHeaders;
	int iStorageBitmapClusterFactor;
	// DeviceType;
	int iVolumeStructureLevel;
	std::string strVolumeName;
	int iVolumeOwnerUIC;
	int iVolumeProtectionCode;
	// VolumeCharacteristics;
	int iDefaultFileProtection;
	int iDefaultWindowSize;
	int iDefaultFileExtend;
	int iDirectoryPreAccessLimit;
	std::string strLastRevision;
	int iCountHomeBlockRevision;
	std::string strVolumeCreationDate;
	int iPackSerialNumber;
	std::string strVolumeOwner;
	std::string strFormatType;

	int bitCount[16] = { 0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4 };
};

