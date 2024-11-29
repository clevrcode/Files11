#include "Files11Record.h"

// Default constructor
File11Record::File11Record(void) : fileNumber(0), fileSeq(0), fileRev(0), headerLBN(0), blockCount(0), bDirectory(false)
{
}

// Constructor
File11Record::File11Record(const char* fname, const char* fext, const char* fcreate, uint16_t fnumber, uint16_t fseq, uint16_t frev, uint32_t lbn, int block_count) :
	fileName(fname), fileExt(fext), fileCreated(fcreate), fileNumber(fnumber), fileSeq(fseq), fileRev(frev), headerLBN(lbn), blockCount(block_count)
{
	bDirectory = (fileExt == "DIR");
	fullName = fileName;
	fullName += ".";
	fullName += fileExt;
}

// Copy constructor
File11Record::File11Record(const File11Record& frec) :
	fileName(frec.fileName), fileExt(frec.fileExt), fileCreated(frec.fileCreated), fullName(frec.fullName), fileNumber(frec.fileNumber), fileSeq(frec.fileSeq),
	fileRev(frec.fileRev), headerLBN(frec.headerLBN), blockCount(frec.blockCount), bDirectory(frec.bDirectory)
{
}

