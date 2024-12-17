#include <assert.h>
#include "Files11HomeBlock.h"
#include "BitCounter.h"

Files11HomeBlock::Files11HomeBlock()
{
	bValid           = false;
	iDiskSize        = 0;
	iScbNbBlocks     = 0;
	iScbUnitSizeBlk  = 0;
	iIndexBitmapSize = 0;
	iIndexBitmapLBN  = 0;
	iIndexFileLBN    = 0;
	iBitmapSysLBN    = 0;
	iBadblkSysLBN    = 0;
	i000000SysLBN    = 0;
	iCorimgSysLBN    = 0;
	iMaxFiles        = 0;
	iUsedHeaders     = 0;

	iStorageBitmapClusterFactor = 0;
	// DeviceType;
	iVolumeStructureLevel = 0;
	iVolumeOwnerUIC       = 0;
	iVolumeProtectionCode = 0;
	// VolumeCharacteristics;
	iDefaultFileProtection   = 0;
	iDefaultWindowSize       = 0;
	iDefaultFileExtend       = 0;
	iDirectoryPreAccessLimit = 0;
	iCountHomeBlockRevision  = 0;
	iPackSerialNumber        = 0;
}

bool Files11HomeBlock::Initialize(const char* diskName)
{
	std::fstream is(diskName, std::fstream::binary);
	if (is) {
		Initialize(is);
		is.close();
	}
	return true;
}

bool Files11HomeBlock::Initialize(std::fstream& istrm)
{
	// get length of file:
	istrm.seekg(0, istrm.end);
	iDiskSize = static_cast<int>(istrm.tellg());
	istrm.seekg(0, istrm.beg);

	ODS1_HomeBlock_t* pHome = ReadHomeBlock(istrm);
	if (pHome != nullptr)
	{
		// home block is valid
		iIndexBitmapSize            = pHome->hm1_w_ibmapsize;
		iIndexBitmapLBN             = (pHome->hm1_w_ibmaplbn_hi << 16) + pHome->hm1_w_ibmaplbn_lo;
		iIndexFileLBN               = iIndexBitmapLBN + iIndexBitmapSize;  // File 1 is the INDEXF.SYS
		iBitmapSysLBN               = iIndexFileLBN + 1; // File 2 is BITMAP.SYS
		iBadblkSysLBN               = iIndexFileLBN + 2; // File 3 is BADBLK.SYS
		i000000SysLBN               = iIndexFileLBN + 3; // File 4 is 000000.SYS
		iCorimgSysLBN               = iIndexFileLBN + 4; // File 5 is CORIMG.SYS
		iMaxFiles                   = pHome->hm1_w_maxfiles;
		iStorageBitmapClusterFactor = pHome->hm1_w_cluster;
		iVolumeStructureLevel       = pHome->hm1_w_structlev;
		iVolumeOwnerUIC             = pHome->hm1_w_volowner;
		iVolumeProtectionCode       = pHome->hm1_w_protect;
		iDefaultFileProtection      = pHome->hm1_w_deffileprot;
		iDefaultWindowSize          = pHome->hm1_b_window;
		iDefaultFileExtend          = pHome->hm1_b_extend;
		iDirectoryPreAccessLimit    = pHome->hm1_b_dirlimit;
		iCountHomeBlockRevision     = pHome->hm1_w_modifcnt;
		iPackSerialNumber           = pHome->hm1_l_serialnum;
			
		MakeString((char *)pHome->hm1_t_volname, sizeof(pHome->hm1_t_volname), strVolumeName, true);
		MakeString((char *)pHome->hm1_t_format, sizeof(pHome->hm1_t_format), strFormatType, true);
		MakeString((char *)pHome->hm1_t_ownername, sizeof(pHome->hm1_t_ownername), strVolumeOwner, true);
		MakeDate(pHome->hm1_t_lastrev, strLastRevision, false);
		MakeDate(pHome->hm1_t_credate, strVolumeCreationDate, true);

		auto pBitmapSysHeader = ReadHeader(iBitmapSysLBN, istrm);
		if (pBitmapSysHeader != nullptr)
		{
			int scb_lbn = 0;
			F11_MapArea_t* pMap = GetMapArea();
			if (pMap->LBSZ == 3) {
				scb_lbn = (pMap->pointers.fm1.hi_lbn << 16) + pMap->pointers.fm1.lo_lbn;
			}
			else if (pMap->LBSZ == 2) {
				scb_lbn = pMap->pointers.fm2.lbn;
			}
			else if (pMap->LBSZ == 4) {
				scb_lbn = (pMap->pointers.fm3.hi_lbn << 16) + pMap->pointers.fm3.lo_lbn;
			}
			if (scb_lbn != 0)
			{
				// Read Storage Control Block
				auto Scb = (SCB_t*)ReadBlock(scb_lbn, istrm);
				if (Scb != nullptr)
				{
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

				BitCounter counter;

				int vbn = 1;
				for (auto lbn = iIndexBitmapLBN; lbn < iIndexFileLBN; lbn++)
				{
					auto pBmp = ReadBlock(lbn, istrm);
					assert(pBmp != nullptr);
					int nbBits = F11_BLOCK_SIZE * 8;
					if (iMaxFiles < (vbn * (F11_BLOCK_SIZE * 8))) {
						nbBits = iMaxFiles % (F11_BLOCK_SIZE * 8);
					}
					counter.Count(pBmp, nbBits);
				}
				//------------------------------------------
				// NOTE: The Index File Bitmap (Ref: 5.1.3)
				//       - bit 1 = header is used
				//       - bit 0 = file number is free
				iUsedHeaders = counter.GetNbHi();
				bValid = true;
			}
		}
	}
	return bValid;
}

bool Files11HomeBlock::ValidateHomeBlock(ODS1_HomeBlock_t* pHome)
{
	uint16_t checksum1 = CalcChecksum((uint16_t*)pHome, (((char*)&pHome->hm1_w_checksum1 - (char*)&pHome->hm1_w_ibmapsize) / 2));
	uint16_t checksum2 = CalcChecksum((uint16_t*)pHome, (((char*)&pHome->hm1_w_checksum2 - (char*)&pHome->hm1_w_ibmapsize) / 2));
	bool bValid = (pHome->hm1_w_checksum1 == checksum1) && (pHome->hm1_w_checksum2 == checksum2);

	bValid &= ((pHome->hm1_w_ibmapsize != 0) && !((pHome->hm1_w_ibmaplbn_hi == 0) && (pHome->hm1_w_ibmaplbn_lo == 0))) &&
		((pHome->hm1_w_structlev == HM1_C_LEVEL1) || (pHome->hm1_w_structlev == HM1_C_LEVEL2)) &&
		(pHome->hm1_w_maxfiles != 0) && (pHome->hm1_w_cluster == 1);
	return bValid;
}

ODS1_HomeBlock_t* Files11HomeBlock::ReadHomeBlock(std::fstream& istrm)
{
	ODS1_HomeBlock_t* pHome = (ODS1_HomeBlock_t*)ReadBlock(F11_HOME_LBN, istrm);
	if (pHome)
	{
		//----------------------
		// Validate home block
		if (!ValidateHomeBlock(pHome))
			pHome = nullptr;
	}
	return pHome;
}

void Files11HomeBlock::PrintInfo(void)
{
	printf("Volume contains a valid ODS1 File system\n");
	printf("Volume Name              : %s\n", strVolumeName.c_str());
	printf("Format                   : %s\n", strFormatType.c_str());
	printf("Maximum number of files  : %d\n", iMaxFiles);
	printf("Total nb of headers used : %d\n", iUsedHeaders);
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

