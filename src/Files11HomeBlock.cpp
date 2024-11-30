#include <assert.h>
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
	iIndexFileLBN    = 0;
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

	ODS1_HomeBlock home;
	assert(sizeof(home) == F11_BLOCK_SIZE);
	if (ReadBlock(home_lbn, istrm, (uint8_t*)&home))
	{
		//----------------------
		// Validate home block

		uint16_t checksum1 = CalcChecksum((uint16_t*)&home, (((char*)&home.hm1_w_checksum1 - (char*)&home.hm1_w_ibmapsize) / 2));
		uint16_t checksum2 = CalcChecksum((uint16_t*)&home, (((char*)&home.hm1_w_checksum2 - (char*)&home.hm1_w_ibmapsize) / 2));
		bValid = (home.hm1_w_checksum1 == checksum1) && (home.hm1_w_checksum2 == checksum2);

		bValid &= ((home.hm1_w_ibmapsize != 0) && !((home.hm1_w_ibmaplbn_hi == 0) && (home.hm1_w_ibmaplbn_lo == 0))) &&
				  ((home.hm1_w_structlev == HM1_C_LEVEL1) || (home.hm1_w_structlev == HM1_C_LEVEL2)) &&
				   (home.hm1_w_maxfiles != 0) &&
				   (home.hm1_w_cluster == 1);

		if (bValid)
		{
			// home block is valid
			iIndexBitmapSize            = home.hm1_w_ibmapsize;
			iIndexBitmapLBN             = (home.hm1_w_ibmaplbn_hi << 16) + home.hm1_w_ibmaplbn_lo;
			iIndexFileLBN               = iIndexBitmapLBN + iIndexBitmapSize;
			iMaxFiles                   = home.hm1_w_maxfiles;
			iStorageBitmapClusterFactor = home.hm1_w_cluster;
			iVolumeStructureLevel       = home.hm1_w_structlev;
			iVolumeOwnerUIC             = home.hm1_w_volowner;
			iVolumeProtectionCode       = home.hm1_w_protect;
			iDefaultFileProtection      = home.hm1_w_deffileprot;
			iDefaultWindowSize          = home.hm1_b_window;
			iDefaultFileExtend          = home.hm1_b_extend;
			iDirectoryPreAccessLimit    = home.hm1_b_dirlimit;
			iCountHomeBlockRevision     = home.hm1_w_modifcnt;
			iPackSerialNumber           = home.hm1_l_serialnum;
			
			MakeString((char *)home.hm1_t_volname, sizeof(home.hm1_t_volname), strVolumeName);
			MakeString((char *)home.hm1_t_format, sizeof(home.hm1_t_format), strFormatType);
			MakeString((char *)home.hm1_t_ownername, sizeof(home.hm1_t_ownername), strVolumeOwner);
			MakeDate(home.hm1_t_lastrev, strLastRevision, false);
			MakeDate(home.hm1_t_credate, strVolumeCreationDate, true);
		}
	}
	return bValid;
}

void Files11HomeBlock::PrintInfo(void)
{
	printf("Volume contains a valid ODS1 File system\n");
	printf("Volume Name             : %s\n", strVolumeName.c_str());
	printf("Format                  : %s\n", strFormatType.c_str());
	printf("Sectors In Volume       : %d\n", 0);
	printf("Maximum number of files : %d\n", iMaxFiles);
	printf("Disk size               : %d\n", 0);
	printf("Volume owner UIC        : %s\n", strVolumeOwner.c_str());
	printf("Volume creation date    : %s\n", strVolumeCreationDate.c_str());
	printf("Bitmap LBN              : %d\n", iIndexBitmapLBN);
	printf("Bitmap Index file size  : %d\n", iIndexBitmapSize);
}

