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

private:
	bool bValid;
	int iIndexBitmapSize;
	int iIndexBitmapLBN;
	int iIndexFileLBN;
	int iMaxFiles;
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


};

