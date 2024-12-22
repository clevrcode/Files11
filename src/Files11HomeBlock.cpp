#include <assert.h>
#include "Files11HomeBlock.h"
#include "BitCounter.h"

Files11HomeBlock::Files11HomeBlock()
{
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
	iVolumeStructureLevel    = 0;
	iVolumeOwnerUIC          = 0;
	iVolumeProtectionCode    = 0;
	// VolumeCharacteristics;
	iDefaultFileProtection   = 0;
	iDefaultWindowSize       = 0;
	iDefaultFileExtend       = 0;
	iDirectoryPreAccessLimit = 0;
	iCountHomeBlockRevision  = 0;
	iPackSerialNumber        = 0;
}

bool Files11HomeBlock::Initialize(std::function<void(int, Files11Base& obj)> fetch)
{
	Files11Base file;
	fetch(F11_HOME_LBN, file);
	F11_HomeBlock_t* pHome = file.GetHome();
	if (!ValidateHomeBlock(pHome))
	{
		return false;
	}

	// home block is valid
	iIndexBitmapSize = pHome->hm1_w_ibmapsize;
	iIndexBitmapLBN = Files11Base::makeLBN(pHome->hm1_w_ibmaplbn_hi, pHome->hm1_w_ibmaplbn_lo);
	iIndexFileLBN = iIndexBitmapLBN + iIndexBitmapSize;  // File 1 is the INDEXF.SYS
	iBitmapSysLBN = iIndexFileLBN + 1; // File 2 is BITMAP.SYS
	iBadblkSysLBN = iIndexFileLBN + 2; // File 3 is BADBLK.SYS
	i000000SysLBN = iIndexFileLBN + 3; // File 4 is 000000.SYS
	iCorimgSysLBN = iIndexFileLBN + 4; // File 5 is CORIMG.SYS
	iMaxFiles = pHome->hm1_w_maxfiles;
	iStorageBitmapClusterFactor = pHome->hm1_w_cluster;
	iVolumeStructureLevel = pHome->hm1_w_structlev;
	iVolumeOwnerUIC = pHome->hm1_w_volowner;
	iVolumeProtectionCode = pHome->hm1_w_protect;
	iDefaultFileProtection = pHome->hm1_w_deffileprot;
	iDefaultWindowSize = pHome->hm1_b_window;
	iDefaultFileExtend = pHome->hm1_b_extend;
	iDirectoryPreAccessLimit = pHome->hm1_b_dirlimit;
	iCountHomeBlockRevision = pHome->hm1_w_modifcnt;
	iPackSerialNumber = pHome->hm1_l_serialnum;

	Files11Base::MakeString((char*)pHome->hm1_t_volname, sizeof(pHome->hm1_t_volname), strVolumeName, true);
	Files11Base::MakeString((char*)pHome->hm1_t_format, sizeof(pHome->hm1_t_format), strFormatType, true);
	Files11Base::MakeString((char*)pHome->hm1_t_ownername, sizeof(pHome->hm1_t_ownername), strVolumeOwner, true);
	Files11Base::MakeDate(pHome->hm1_t_lastrev, strLastRevision, false);
	Files11Base::MakeDate(pHome->hm1_t_credate, strVolumeCreationDate, true);

	fetch(iBitmapSysLBN, file);
	if (!file.ValidateHeader())
	{
		return false;
	}
	// Read Storage Control Block
	Files11Base::BlockList_t blkList;
	Files11Base::GetBlockPointers(file.GetMapArea(), blkList);
	// The SCB is the first block of the Bitmap.sys file
	if (blkList.size() > 0)
	{
		int scb_lbn = blkList[0].lbn_start;
		fetch(scb_lbn, file);
		SCB_t* pScb = (SCB_t*)file.GetSCB();
		iScbNbBlocks = pScb->nbBitmapBlks;
		if (iScbNbBlocks < 127) {
			uint8_t* pBegin = (uint8_t*)&pScb->blocks.smallUnit;
			uint16_t* pSize = (uint16_t*)(pBegin + (iScbNbBlocks * sizeof(SCB_DataFreeBlks_t)));
			iScbUnitSizeBlk = (pSize[0] << 16) + pSize[1];
		}
		else
		{
			iScbUnitSizeBlk = (pScb->blocks.largeUnit.unitSizeLogBlks_hi << 16) + pScb->blocks.largeUnit.unitSizeLogBlks_lo;
		}

		BitCounter counter;

		int vbn = 1;
		for (auto lbn = iIndexBitmapLBN; lbn < iIndexFileLBN; lbn++, vbn++)
		{
			fetch(lbn, file);
			auto pBmp = file.GetBlock();
			assert(pBmp != nullptr);
			int nbBits = F11_BLOCK_SIZE * 8;
			if (iMaxFiles < (vbn * (F11_BLOCK_SIZE * 8)))
				nbBits = iMaxFiles % (F11_BLOCK_SIZE * 8);
			counter.Count(pBmp, nbBits);
		}
		//------------------------------------------
		// NOTE: The Index File Bitmap (Ref: 5.1.3)
		//       - bit 1 = header is used
		//       - bit 0 = file number is free
		iUsedHeaders = counter.GetNbHi();
	}
	return true;
}

bool Files11HomeBlock::ValidateHomeBlock(F11_HomeBlock_t* pHome)
{
	uint16_t checksum1 = Files11Base::CalcChecksum((uint16_t*)pHome, (((char*)&pHome->hm1_w_checksum1 - (char*)&pHome->hm1_w_ibmapsize) / 2));
	uint16_t checksum2 = Files11Base::CalcChecksum((uint16_t*)pHome, (((char*)&pHome->hm1_w_checksum2 - (char*)&pHome->hm1_w_ibmapsize) / 2));
	bool bValid = (pHome->hm1_w_checksum1 == checksum1) && (pHome->hm1_w_checksum2 == checksum2);

	bValid &= ((pHome->hm1_w_ibmapsize != 0) && !((pHome->hm1_w_ibmaplbn_hi == 0) && (pHome->hm1_w_ibmaplbn_lo == 0))) &&
		((pHome->hm1_w_structlev == HM1_C_LEVEL1) || (pHome->hm1_w_structlev == HM1_C_LEVEL2)) &&
		(pHome->hm1_w_maxfiles != 0) && (pHome->hm1_w_cluster == 1);
	return bValid;
}

void Files11HomeBlock::PrintInfo(void)
{
	printf("Volume contains a valid ODS1 File system\n");
	printf("Volume Name              : %s\n", strVolumeName.c_str());
	printf("Format                   : %s\n", strFormatType.c_str());
	printf("Maximum number of files  : %d\n", iMaxFiles);
	printf("Total nb of headers used : %d\n", iUsedHeaders);
	printf("Blocks In Volume         : %d\n", iScbUnitSizeBlk);
	printf("Disk size                : %d\n", iScbUnitSizeBlk * F11_BLOCK_SIZE);
	printf("Volume owner UIC         : %s\n", strVolumeOwner.c_str());
	printf("Volume creation date     : %s\n", strVolumeCreationDate.c_str());
	printf("Home block last rev date : %s\n", strLastRevision.c_str());
	printf("Home block modif count   : %d\n", iCountHomeBlockRevision);
	printf("Bitmap LBN               : %d (0x%08X)\n", iIndexBitmapLBN, iIndexBitmapLBN);
	printf("Bitmap Index file size   : %d\n", iIndexBitmapSize);
}

