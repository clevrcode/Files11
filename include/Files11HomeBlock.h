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
	int  GetMaxFiles(void) { return iMaxFiles; };
	int  GetIndexLBN(void) { return iIndexFileLBN; };
	int  GetDiskSize(void) { return bValid ? iDiskSize : 0;  };
	int  CountTotalFiles(std::ifstream& istrm);
	int  CountUsedHeaders(std::ifstream& istrm);

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

