#include "Files11HomeBlock.h"

Files11HomeBlock::Files11HomeBlock()
{
	//std::string strVolumeName;
	//std::string strLastRevision;
	//std::string strVolumeCreationDate;
	//std::string strVolumeName;
	//std::string strVolumeOwner;
	//std::string strFormatType;
	bValid = false;
	iIndexBitmapSize = 0;
	iIndexBitmapLBN  = 0;
	iMaxFiles        = 0;
	iStorageBitmapClusterFactor = 0;
	// DeviceType;
	iVolumeStructureLevel = 0;
	iVolumeOwnerUIC = 0;
	iVolumeProtectionCode = 0;
	// VolumeCharacteristics;
	iDefaultFileProtection = 0;
	iDefaultWindowSize = 0;
	iDefaultFileExtend = 0;
	iDirectoryPreAccessLimit = 0;
	iCountHomeBlockRevision = 0;
	iPackSerialNumber = 0;
}

bool Files11HomeBlock::Initialize(const char* diskName)
{
	std::ifstream is(diskName, std::ifstream::binary);
	if (is) {
		Initialize(is);
		is.close();
	}
	return true;
}

bool Files11HomeBlock::Initialize(std::ifstream& istrm)
{
	int home_lbn = F11_HOME_LBN * F11_BLOCK_SIZE;
	istrm.seekg(home_lbn, istrm.beg);

	ODS1_HomeBlock home;
	istrm.read((char *)&home, sizeof(home));
	if (istrm.good())
	{
		//----------------------
		// Validate home block

		uint16_t checksum1 = CalcChecksum((uint16_t*)&home, (((char*)&home.hm1_w_checksum1 - (char*)&home.hm1_w_ibmapsize) / 2));
		uint16_t checksum2 = CalcChecksum((uint16_t*)&home, (((char*)&home.hm1_w_checksum2 - (char*)&home.hm1_w_ibmapsize) / 2));
		bValid = (home.hm1_w_checksum1 == checksum1) && (home.hm1_w_checksum2 == checksum2);

		bValid = ((home.hm1_w_ibmapsize != 0) && !((home.hm1_w_ibmaplbn_hi == 0) && (home.hm1_w_ibmaplbn_lo == 0))) &&
				 ((home.hm1_w_structlev == HM1_C_LEVEL1) || (home.hm1_w_structlev == HM1_C_LEVEL2)) &&
				  (home.hm1_w_maxfiles != 0) &&
				  (home.hm1_w_cluster == 1) &&
				  (home.hm1_w_checksum1 == checksum1) &&
			      (home.hm1_w_checksum2 != checksum2);

		if (bValid)
		{
			// home block is valid


		}
	}
	return bValid;
}

