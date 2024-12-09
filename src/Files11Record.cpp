#include <assert.h>
#include "Files11Record.h"

// Default constructor
Files11Record::Files11Record(int IndexLBN/*=0*/)
{
	firstLBN = IndexLBN;
	fileNumber = 0;
	fileSeq = 0;
	fileVersion = 0;
	fileRevision = 0;
	ownerUIC = 0;
	fileProtection = 0;
	sysCharacteristics = 0;
	userCharacteristics = 0;
	bDirectory = false;
	headerLBN  = 0;
	blockCount = 0;
    fileExtensionSegment = 0;
}

// Copy constructor
Files11Record::Files11Record(const Files11Record& frec) :
	firstLBN(frec.firstLBN),
	fileName(frec.fileName), fileExt(frec.fileExt), fileCreationDate(frec.fileCreationDate), fullName(frec.fullName),
	fileNumber(frec.fileNumber), fileSeq(frec.fileSeq), fileVersion(frec.fileVersion), fileRevision(frec.fileRevision),
	ownerUIC(frec.ownerUIC), fileProtection(frec.fileProtection), headerLBN(frec.headerLBN),
	sysCharacteristics(frec.sysCharacteristics), userCharacteristics(frec.userCharacteristics),
	blockCount(frec.blockCount), bDirectory(frec.bDirectory), fileFCS(frec.fileFCS), fileExtensionSegment(frec.fileExtensionSegment)
{
}

// Initialization
// Return the file number, 0 if header is not valid

int Files11Record::Initialize(int lbn, std::fstream &istrm)
{
	auto pHdr = (ODS1_FileHeader_t*)ReadFileHeader(lbn, istrm);
	if (pHdr != nullptr)
	{
		fileNumber = pHdr->fh1_w_fid_num;
		if (fileNumber != 0)
		{
			assert(pHdr->fh1_b_idoffset == F11_HEADER_FID_OFFSET);
			assert(pHdr->fh1_b_mpoffset == F11_HEADER_MAP_OFFSET);

			// If the header is a continuation segment, skip it
			F11_MapArea_t* pMap = GetMapArea();
            fileExtensionSegment = pMap->ext_SegNumber;

			fileSeq    = pHdr->fh1_w_fid_seq;
			ownerUIC   = pHdr->fh1_w_fileowner;
			fileProtection = pHdr->fh1_w_fileprot;
			sysCharacteristics = pHdr->fh1_b_syschar;
			userCharacteristics = pHdr->fh1_b_userchar;
			fileFCS.Initialize((ODS1_UserAttrArea_t*) & pHdr->fh1_w_ufat);

			F11_IdentArea_t* pIdent = GetIdentArea();
			if (pIdent)
			{
				//fileRev = pRecord->fileRVN;
				Radix50ToAscii(pIdent->filename, 3, fileName, true);
				Radix50ToAscii(pIdent->filetype, 1, fileExt, true);
				fullName = fileName + "." + fileExt;
				MakeDate(pIdent->revision_date, fileRevisionDate, true);
				MakeDate(pIdent->creation_date, fileCreationDate, true);
				MakeDate(pIdent->expiration_date, fileExpirationDate, false);
				bDirectory = (fileExt == "DIR");
				headerLBN = lbn;
				blockCount = BuildBlockList(lbn, blockList, istrm);
				blockCountString = std::to_string(blockCount);
				blockCountString += ".";
			}
			else
				return 0;
		}
		return fileNumber;
	}
	return 0;
}

const char* Files11Record::GetFileCreation(bool no_seconds /*=true*/) const
{
	return fileCreationDate.c_str(); 
};

const char* Files11Record::GetBlockCountString(void) const
{
	return blockCountString.c_str();
}

bool Files11Record::ValidateHeader(ODS1_FileHeader_t* pHeader)
{
	uint16_t checksum = CalcChecksum((uint16_t*)pHeader, 255);
	return (checksum == pHeader->fh1_w_checksum);
}


ODS1_FileHeader_t* Files11Record::ReadFileHeader(int lbn, std::fstream& istrm)
{
	ODS1_FileHeader_t* pHeader = (ODS1_FileHeader_t*) ReadBlock(lbn, istrm);
	if (pHeader)
	{
		if (!ValidateHeader(pHeader))
			pHeader = nullptr;
	}
	return pHeader;
}

int Files11Record::BuildBlockList(int lbn, BlockList_t& blk_list, std::fstream& istrm)
{
    int count = 0;
    uint8_t expected_segment = 0;
    int current_lbn = lbn;

    // Clear the plock list
    blk_list.clear();

    do {
        ODS1_FileHeader_t Header;
        if (readBlock(current_lbn, istrm, (uint8_t*)&Header))
        {
            F11_MapArea_t* pMap = (F11_MapArea_t*)((uint16_t*)&Header + Header.fh1_b_mpoffset);
            if (pMap->ext_SegNumber != expected_segment)
            {
                fprintf(stderr, "WARNING: Invalid segment (lbn: %d): expected %d, read %d\n", lbn, expected_segment, pMap->ext_SegNumber);
            }
            count += GetBlockCount(pMap, &blk_list);

            if (pMap->ext_FileNumber > 0)
            {
                current_lbn = (pMap->ext_FileNumber - 1) + firstLBN;
                expected_segment++;
            }
            else
                current_lbn = 0;
        }
        else
        {
            fprintf(stderr, "Failed to reab block lbn %d\n", current_lbn);
            break;
        }
    } while (current_lbn > 0);
    return count;
}

int Files11Record::GetBlockCount(F11_MapArea_t* pMap, BlockList_t* pBlkList/*=nullptr*/)
{
    int count = 0;
    int ptr_size = 0;

    if (pMap->LBSZ == 3)
        ptr_size = sizeof(pMap->pointers.fm1);
    else if (pMap->LBSZ == 2)
        ptr_size = sizeof(pMap->pointers.fm2);
    else if (pMap->LBSZ == 4)
        ptr_size = sizeof(pMap->pointers.fm3);
    else
        return 0;

    ptr_size /= 2;
    int nbPtrs = pMap->USE / ptr_size;
    uint16_t* p = (uint16_t*)&pMap->pointers;

    for (int i = 0; i < nbPtrs; i++)
    {
        PtrsFormat_t* ptrs = (PtrsFormat_t*)p;
        BlockPtrs_t blocks;
        if (pMap->LBSZ == 3)
        {
            count += ptrs->fm1.blk_count + 1;
            if (pBlkList) {
                blocks.lbn_start = (ptrs->fm1.hi_lbn << 16) + ptrs->fm1.lo_lbn;
                blocks.lbn_end = blocks.lbn_start + ptrs->fm1.blk_count;
                pBlkList->push_back(blocks);
            }
        }
        else if (pMap->LBSZ == 2)
        {
            count += ptrs->fm2.blk_count + 1;
            if (pBlkList) {
                blocks.lbn_start = ptrs->fm2.lbn;
                blocks.lbn_end = blocks.lbn_start + ptrs->fm2.blk_count;
                pBlkList->push_back(blocks);
            }
        }
        else if (pMap->LBSZ == 4) {
            count += ptrs->fm3.blk_count + 1;
            if (pBlkList) {
                blocks.lbn_start = (ptrs->fm3.hi_lbn << 16) + ptrs->fm3.lo_lbn;
                blocks.lbn_end = blocks.lbn_start + ptrs->fm3.blk_count;
                pBlkList->push_back(blocks);
            }
        }
        p += ptr_size;
    }
    return count;
}

