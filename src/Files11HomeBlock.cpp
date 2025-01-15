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

bool Files11HomeBlock::Initialize(F11_HomeBlock_t* pHome)
{
	if (!ValidateHomeBlock(pHome))
		return false;

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
	return true;
}

bool Files11HomeBlock::InitializeSCB(SCB_t* pSCB)
{
	iScbNbBlocks = pSCB->nbBitmapBlks;
	if (iScbNbBlocks < 127) {
		uint8_t* pBegin = (uint8_t*)&pSCB->blocks.smallUnit;
		uint16_t* pSize = (uint16_t*)(pBegin + (iScbNbBlocks * sizeof(SCB_DataFreeBlks_t)));
		iScbUnitSizeBlk = (pSize[0] << 16) + pSize[1];
	}
	else
	{
		iScbUnitSizeBlk = (pSCB->blocks.largeUnit.unitSizeLogBlks_hi << 16) + pSCB->blocks.largeUnit.unitSizeLogBlks_lo;
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
	const char* BOLD      = "\x1b[1m";
	const char* NOBOLD    = "\x1b[22m";
	const char* WHITE     = "\x1b[37m";
	const char* MAGENTA   = "\x1b[35m";
	const char* GREEN     = "\x1b[32m";
	const char* YELLOW    = "\x1b[33m";
	const char* UNDERLINE = "\x1b[4m";
	const char* NOUNDERLN = "\x1b[24m";
	const char* DEFAULT   = "\x1b[0m";

	std::cout << UNDERLINE << BOLD << WHITE << "                                              " << NOUNDERLN << NOBOLD << std::endl;
	std::cout << BOLD << WHITE << "Volume contains a valid ODS1 File system" << NOBOLD << std::endl;
	std::cout << WHITE << "Volume Name              : " << YELLOW << strVolumeName   << std::endl;
	std::cout << WHITE << "Format                   : " << YELLOW << strFormatType   << std::endl;
	std::cout << WHITE << "Maximum number of files  : " << YELLOW << iMaxFiles       << std::endl;
	std::cout << WHITE << "Total nb of headers used : " << YELLOW << iUsedHeaders    << std::endl;
	std::cout << WHITE << "Blocks In Volume         : " << YELLOW << iScbUnitSizeBlk << std::endl;
	std::cout << WHITE << "Disk size                : " << YELLOW << iScbUnitSizeBlk * F11_BLOCK_SIZE << std::endl;
	std::cout << WHITE << "Volume owner UIC         : " << YELLOW << strVolumeOwner  << std::endl;
	std::cout << WHITE << "Volume creation date     : " << YELLOW << strVolumeCreationDate << std::endl;
	std::cout << WHITE << "Home block last rev date : " << YELLOW << strLastRevision << std::endl;
	std::cout << WHITE << "Home block modif count   : " << YELLOW << iCountHomeBlockRevision << std::endl;
	std::cout << WHITE << "Bitmap LBN               : " << YELLOW << iIndexBitmapLBN << " (0x" << std::hex << iIndexBitmapLBN << ")" << std::dec << std::endl;
	std::cout << WHITE << "Bitmap Index file size   : " << YELLOW << iIndexBitmapSize << std::endl;
	std::cout << UNDERLINE << BOLD << WHITE << "                                              " << NOUNDERLN << NOBOLD << std::endl;
	std::cout << DEFAULT;
}

