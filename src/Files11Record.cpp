#include <assert.h>
#include "Files11Record.h"

// Default constructor
Files11Record::Files11Record(int bitmapLBN/*=0*/)
{
	firstLBN = bitmapLBN;
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
}

// Copy constructor
Files11Record::Files11Record(const Files11Record& frec) :
	firstLBN(frec.firstLBN),
	fileName(frec.fileName), fileExt(frec.fileExt), fileCreationDate(frec.fileCreationDate), fullName(frec.fullName),
	fileNumber(frec.fileNumber), fileSeq(frec.fileSeq), fileVersion(frec.fileVersion), fileRevision(frec.fileRevision),
	ownerUIC(frec.ownerUIC), fileProtection(frec.fileProtection), headerLBN(frec.headerLBN),
	sysCharacteristics(frec.sysCharacteristics), userCharacteristics(frec.userCharacteristics),
	blockCount(frec.blockCount), bDirectory(frec.bDirectory), fileFCS(frec.fileFCS)
{
}

// Initialization
// Return the file number, 0 if header is not valid

int Files11Record::Initialize(int lbn, std::ifstream &istrm)
{
	auto pHdr = (ODS1_FileHeader_t*)ReadFileHeader(lbn, istrm);
	if (pHdr != nullptr)
	{
		fileNumber = pHdr->fh1_w_fid_num;
		if (fileNumber != 0)
		{

			assert(pHdr->fh1_b_idoffset == 0x17);
			assert(pHdr->fh1_b_mpoffset == 0x2e);

			// If the header is a continuation segment, skip it
			F11_MapArea_t* pMap = GetMapArea();
			if (pMap->ext_SegNumber != 0)
				return 0;

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


ODS1_FileHeader_t* Files11Record::ReadFileHeader(int lbn, std::ifstream& istrm)
{
	ODS1_FileHeader_t* pHeader = (ODS1_FileHeader_t*) ReadBlock(lbn, istrm);
	if (pHeader)
	{
		if (!ValidateHeader(pHeader))
			pHeader = nullptr;
	}
	return pHeader;
}
