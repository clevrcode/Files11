#include <assert.h>
#include "Files11HomeBlock.h"

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
	iMaxFiles        = 0;
	iTotalFiles      = 0;
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

	ODS1_HomeBlock_t* pHome = ReadHomeBlock(istrm);
	if (pHome != NULL)
	{
		// home block is valid
		iIndexBitmapSize            = pHome->hm1_w_ibmapsize;
		iIndexBitmapLBN             = (pHome->hm1_w_ibmaplbn_hi << 16) + pHome->hm1_w_ibmaplbn_lo;
		iIndexFileLBN               = iIndexBitmapLBN + iIndexBitmapSize; 
		iBitmapSysLBN               = iIndexFileLBN + 1; 
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
			
		MakeString((char *)pHome->hm1_t_volname, sizeof(pHome->hm1_t_volname), strVolumeName);
		MakeString((char *)pHome->hm1_t_format, sizeof(pHome->hm1_t_format), strFormatType);
		MakeString((char *)pHome->hm1_t_ownername, sizeof(pHome->hm1_t_ownername), strVolumeOwner);
		MakeDate(pHome->hm1_t_lastrev, strLastRevision, false);
		MakeDate(pHome->hm1_t_credate, strVolumeCreationDate, true);

		ODS1_FileHeader_t* pBitmapSysHeader = ReadFileHeader(iBitmapSysLBN, istrm);
		if (pBitmapSysHeader != NULL)
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
				SCB_t* Scb = (SCB_t*)ReadBlock(scb_lbn, istrm);
				if (Scb != NULL)
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
				iTotalFiles = CountTotalFiles(istrm);
				iUsedHeaders = CountUsedHeaders(istrm);
				bValid = true;
			}
		}
	}
	return bValid;
}

int Files11HomeBlock::CountUsedHeaders(std::ifstream& istrm)
{
	int count = 0;
	int start_lbn = iIndexBitmapLBN + iIndexBitmapSize;
	int end_lbn = start_lbn + iMaxFiles;
	for (int lbn = start_lbn; lbn < end_lbn; lbn++)
	{
		ODS1_FileHeader_t* pHeader = ReadFileHeader(lbn, istrm);
		if (pHeader)
		{
			if (pHeader->fh1_w_fid_num != 0)
				count++;
		}
	}
	return count;
}


int Files11HomeBlock::CountTotalFiles(std::ifstream& istrm)
{
	int fileCount = 0;
	for (int i = 0; i < iIndexBitmapSize; i++)
	{
		uint8_t* pBlock = ReadBlock(iIndexBitmapLBN + i, istrm);
		if (pBlock)
		{
			for (int b = 0; b < F11_BLOCK_SIZE; b++)
			{
				fileCount += bitCount[pBlock[b] % 16];
				fileCount += bitCount[pBlock[b] / 16];
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
	printf("Total nb of headers used : %d, %d\n", iTotalFiles, iUsedHeaders);
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

