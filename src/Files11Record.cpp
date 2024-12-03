#include "Files11Record.h"

// Default constructor
Files11Record::Files11Record(void)
{
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
	ODS1_FileHeader_t* pHdr = ReadFileHeader(lbn, istrm);
	if (pHdr)
	{
		// If the header is a continuation segment, skip it
		F11_MapArea_t* pMap = GetMapArea();
		if (pMap->ext_SegNumber != 0)
			return 0;

		fileNumber = pHdr->fh1_w_fid_num;
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
			fullName = fileName + "." + fileExt + ";";
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

		// Everything is OK, return the file number
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

