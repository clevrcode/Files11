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
	iDiskSize = 0;
	iScbNbBlocks = 0;
	iScbUnitSizeBlk = 0;
	iIndexBitmapSize = 0;
	iIndexBitmapLBN  = 0;
	iIndexFileLBN    = 0;
	iMaxFiles        = 0;
	iTotalFiles      = 0;
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
	// get length of file:
	istrm.seekg(0, istrm.end);
	iDiskSize = static_cast<int>(istrm.tellg());
	istrm.seekg(0, istrm.beg);

	ODS1_HomeBlock home;
	assert(sizeof(home) == F11_BLOCK_SIZE);
	if (ReadBlock(F11_HOME_LBN, istrm, (uint8_t*)&home))
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
			iBitmapSysLBN               = iIndexFileLBN + 1; 
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

			ODS1_FileHeader BitmapSysHeader;
			if (ReadFileHeader(iBitmapSysLBN, istrm, &BitmapSysHeader))
			{
				int scb_lbn = 0;
				F11_MapArea_t* pMap = (F11_MapArea_t*)((uint16_t*)&BitmapSysHeader + BitmapSysHeader.fh1_b_mpoffset);
				if (pMap->LBSZ == 3) {
					scb_lbn = (pMap->pointers.fm1.hi_lbn << 16) + pMap->pointers.fm1.lo_lbn;
				}
				else if (pMap->LBSZ == 2) {
					scb_lbn = pMap->pointers.fm2.lbn;
				}
				else if (pMap->LBSZ == 4) {
					scb_lbn = (pMap->pointers.fm3.hi_lbn << 16) + pMap->pointers.fm3.lo_lbn;
				}
				else {
					// ERROR
					bValid = false;
				}
				if (bValid)
				{
					// Read Storage Control Block
					uint8_t block[F11_BLOCK_SIZE];
					if (ReadBlock(scb_lbn, istrm, block))
					{
						SCB_t *Scb = (SCB_t *)block;
						iScbNbBlocks = Scb->nbBitmapBlks;
						if (iScbNbBlocks < 127) {
							uint8_t*  pBegin = (uint8_t*)&Scb->blocks.smallUnit;
							uint16_t *pSize  = (uint16_t*)(pBegin + (iScbNbBlocks * sizeof(SCB_DataFreeBlks_t)));
							iScbUnitSizeBlk = (pSize[0] << 16) + pSize[1];
						}
						else {
							iScbUnitSizeBlk = (Scb->blocks.largeUnit.unitSizeLogBlks_hi << 16) + Scb->blocks.largeUnit.unitSizeLogBlks_lo;
						}
					}

					iTotalFiles = CountTotalFiles(istrm);

				}

			}
		}
	}
	return bValid;
}

int Files11HomeBlock::CountTotalFiles(std::ifstream& istrm)
{
	int fileCount = 0;
	if (bValid)
	{
		uint8_t block[F11_BLOCK_SIZE];
		for (int i = 0; i < iIndexBitmapSize; i++)
		{
			if (ReadBlock(iIndexBitmapLBN + i, istrm, block))
			{
				for (int b = 0; b < F11_BLOCK_SIZE; b++)
				{
					fileCount += bitCount[block[b] % 16];
					fileCount += bitCount[block[b] / 16];
				}
			}
		}
	}
	return fileCount;
}

void Files11HomeBlock::PrintInfo(void)
{
	printf("Volume contains a valid ODS1 File system\n");
	printf("Volume Name              : %s\n", strVolumeName.c_str());
	printf("Format                   : %s\n", strFormatType.c_str());
	printf("Maximum number of files  : %d\n", iMaxFiles);
	printf("Total nb of headers used : %d\n", iTotalFiles);
	printf("Blocks In Volume         : %d\n", (iDiskSize / F11_BLOCK_SIZE) - 1); // Do not count the boot block
	printf("Disk size                : %d\n", iDiskSize);
	printf("Unit size in blocks      : %d\n", iScbUnitSizeBlk);
	printf("Volume owner UIC         : %s\n", strVolumeOwner.c_str());
	printf("Volume creation date     : %s\n", strVolumeCreationDate.c_str());
	printf("Home block last rev date : %s\n", strLastRevision.c_str());
	printf("Home block modif count   : %d\n", iCountHomeBlockRevision);
	printf("Bitmap LBN               : %d (0x%08X)\n", iIndexBitmapLBN, iIndexBitmapLBN);
	printf("Bitmap Index file size   : %d\n", iIndexBitmapSize);
}

