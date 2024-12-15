#pragma once

#include <stdint.h>
#include <string>
#include <vector>
#include "Files11_defs.h"
#include "Files11Base.h"
#include "Files11FCS.h"

class Files11Record : public Files11Base 
{
public:
	Files11Record(void);
	Files11Record(const Files11Record&);

	int      Initialize(int lbn, std::fstream& istrm);
	uint16_t GetFileNumber(void) const          { return fileNumber;                  };
	uint16_t GetFileSeq(void) const             { return fileSeq;                     };
	uint16_t GetFileRevision(void) const        { return fileRevision;                };
	uint32_t GetHeaderLBN(void) const           { return headerLBN;                   };
	int      GetUsedBlockCount(void)            { return fileFCS.GetUsedBlockCount(); };
	bool	 IsDirectory(void) const            { return bDirectory;                  };
	bool     IsFileExtension(void) const        { return fileExtensionSegment != 0;   };
	bool     IsContiguous(void) const           { return (userCharacteristics & uc_con) != 0; };
	bool     IsMarkedForDeletion(void) const    { return (sysCharacteristics & sc_mdl) != 0;  };
	const Files11FCS& GetFileFCS(void) const    { return fileFCS;                     };
	const char* GetFileName(void) const         { return fileName.c_str();            };
	const char* GetFileExt(void) const          { return fileExt.c_str();             };
	const char* GetFullName(void) const         { return fullName.c_str();            };
	const char* GetFullName(int version)  { 
		char octbuf[32];
		snprintf(octbuf, sizeof(octbuf), "%o", version);
		fullNameWithVersion = fullName + ";" + octbuf;
		return fullNameWithVersion.c_str(); 
	};
	void           PrintRecord(void);
	const char*    GetFileCreation(bool no_seconds = true) const;
	const char*    GetFileRevisionDate(void)   const { return fileRevisionDate.c_str(); };
	const uint16_t GetOwnerUIC(void)           const { return ownerUIC;                 };
	const uint16_t GetFileProtection(void)     const { return fileProtection;           };

protected:
	bool ValidateHeader(ODS1_FileHeader_t* pHeader);
	ODS1_FileHeader_t* ReadFileHeader(int lbn, std::fstream& istrm);

private:
	int         fileExtensionSegment;
	uint16_t	fileNumber;
	uint16_t	fileSeq;
	uint16_t	fileVersion;
	uint16_t	fileRevision;
	uint16_t	ownerUIC;
	uint16_t    fileProtection;
	uint8_t		sysCharacteristics;
	uint8_t		userCharacteristics;
	Files11FCS	fileFCS;
	std::string	fileName;
	std::string	fileExt;
	std::string fileCreationDate;
	std::string fileRevisionDate;
	std::string fileExpirationDate;
	std::string fullName;
	std::string fullNameWithVersion;
	bool		bDirectory;
	uint32_t	headerLBN;
};

