#include <assert.h>
#include "Files11Record.h"

// Default constructor
Files11Record::Files11Record(void)
{
	fileNumber           = 0;
	fileSeq              = 0;
	fileVersion          = 0;
	fileRevision         = 0;
	ownerUIC             = 0;
	fileProtection       = 0;
	sysCharacteristics   = 0;
	userCharacteristics  = 0;
	bDirectory           = false;
	headerLBN            = 0;
    fileExtensionSegment = 0;
}

// Copy constructor
Files11Record::Files11Record(const Files11Record& frec) :
	fileName(frec.fileName), fileExt(frec.fileExt), fileCreationDate(frec.fileCreationDate), fullName(frec.fullName),
	fileNumber(frec.fileNumber), fileSeq(frec.fileSeq), fileVersion(frec.fileVersion), fileRevision(frec.fileRevision),
	ownerUIC(frec.ownerUIC), fileProtection(frec.fileProtection), headerLBN(frec.headerLBN),
	sysCharacteristics(frec.sysCharacteristics), userCharacteristics(frec.userCharacteristics),
	bDirectory(frec.bDirectory), fileFCS(frec.fileFCS), fileExtensionSegment(frec.fileExtensionSegment)
{
}

// Initialization
// Return the file number, 0 if header is not valid

int Files11Record::Initialize(int lbn, std::fstream &istrm)
{
	auto pHdr = (F11_FileHeader_t*)ReadFileHeader(lbn, istrm);
	if (pHdr != nullptr)
	{
		fileNumber = pHdr->fh1_w_fid_num;
		if (fileNumber != 0)
		{
			assert(pHdr->fh1_b_idoffset == F11_HEADER_FID_OFFSET);
			assert(pHdr->fh1_b_mpoffset == F11_HEADER_MAP_OFFSET);

			// If the header is a continuation segment, skip it
			F11_MapArea_t* pMap  = m_File.GetMapArea();
            fileExtensionSegment = pMap->ext_SegNumber;
			fileSeq              = pHdr->fh1_w_fid_seq;
			ownerUIC             = pHdr->fh1_w_fileowner;
			fileProtection       = pHdr->fh1_w_fileprot;
			sysCharacteristics   = pHdr->fh1_b_syschar;
			userCharacteristics  = pHdr->fh1_b_userchar;
			fileFCS.Initialize((F11_UserAttrArea_t*) & pHdr->fh1_w_ufat);

			F11_IdentArea_t* pIdent = m_File.GetIdentArea();
			if (pIdent)
			{
				//fileRev = pRecord->fileRVN;
				Files11Base::Radix50ToAscii(pIdent->filename, 3, fileName, true);
				Files11Base::Radix50ToAscii(pIdent->filetype, 1, fileExt, true);
                fullName = fileName;
                if (fileExt.length() > 0)
    				fullName = fileName + "." + fileExt;
				Files11Base::MakeDate(pIdent->revision_date, fileRevisionDate, true);
				Files11Base::MakeDate(pIdent->creation_date, fileCreationDate, true);
				Files11Base::MakeDate(pIdent->expiration_date, fileExpirationDate, false);
				bDirectory = (fileExt == "DIR") && (fileFCS.GetRecordSize() == 16) && (fileExtensionSegment == 0);
				fileVersion = pIdent->version;
				fileRevision = pIdent->revision;
				headerLBN = lbn;
			}
			else
				return 0;
		}
		return fileNumber;
	}
	return 0;
}

void Files11Record::Refresh(std::fstream& istrm)
{
	Initialize(headerLBN, istrm);
}

F11_FileHeader_t* Files11Record::ReadFileHeader(int lbn, std::fstream& istrm)
{
	F11_FileHeader_t* pHeader = (F11_FileHeader_t*) m_File.ReadBlock(lbn, istrm);
	if (pHeader)
	{
		if (!m_File.ValidateHeader())
			pHeader = nullptr;
	}
	return pHeader;
}

void Files11Record::PrintRecord(int version)
{
	std::cout.fill(' ');	
	std::cout.width(20); std::cout << std::left << GetFullName(version);
	std::string blks(std::to_string(GetUsedBlockCount()));
	blks += ".";
	std::cout.width(8);	std::cout << blks << (IsContiguous() ? "C  " : "   ") << GetFileCreation() << std::endl;
}

void Files11Record::ListRecord(std::ostream& os)
{
	char buffer[1024];

	// print filename.ext blocks used.
	std::string str(std::to_string(GetUsedBlockCount()));
	str += ".";
	snprintf(buffer, sizeof(buffer), "%-20s%-8s%s ", GetFullName(fileVersion), str.c_str(), GetFileCreation());
	os << buffer;
	// print (filenumber,sequence) headerLBN
	str = " (";
	str += std::to_string(fileNumber);
	str += ',';
	str += std::to_string(fileSeq);
	str += ")";
	snprintf(buffer, sizeof(buffer), "%-16s%-10d", str.c_str(), headerLBN);
	os << buffer;

	// print file owner, protection
	Files11Base::MakeUIC((uint8_t*)&ownerUIC, str);
	std::string fprot;
	Files11Base::GetFileProtectionString(fileProtection, fprot);
	snprintf(buffer, sizeof(buffer), "%-10s%s", str.c_str(), fprot.c_str());
	os << buffer << std::endl;
}

