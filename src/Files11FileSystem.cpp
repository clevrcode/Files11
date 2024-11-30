#include "Files11FileSystem.h"
#include "Files11_defs.h"


// Constructor
Files11FileSystem::Files11FileSystem()
{
    iDiskSize = 0;
}

Files11FileSystem::~Files11FileSystem()
{
}

bool Files11FileSystem::Open(const char *dskName)
{
    std::ifstream is(dskName, std::ifstream::binary);
    if (is) {
        // get length of file:
        is.seekg(0, is.end);
        iDiskSize = is.tellg();
        is.seekg(0, is.beg);

        // Read the Home Block
        ODS1_HomeBlock HomeBlock;
        readBlock(F11_HOME_LBN, (char*)&HomeBlock, is);

        is.close();
    }

	return false;
}


//=============================================================================
//  L O W   L E V E L    S E R V I C E S

bool Files11FileSystem::readBlock(int lbn, char *pBlk, std::ifstream& istrm)
{
    istrm.seekg(0, istrm.beg);
    istrm.read(pBlk, F11_BLOCK_SIZE);
    return istrm.good();
}