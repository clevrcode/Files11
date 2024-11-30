#include "Files11FileSystem.h"
#include "Files11_defs.h"


// Constructor
Files11FileSystem::Files11FileSystem()
{
    m_iDiskSize = 0;
    m_bValid = false;
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
        m_iDiskSize = is.tellg();
        is.seekg(0, is.beg);

        // Read the Home Block
        m_bValid = m_HomeBlock.Initialize(is);
        is.close();
    }
	return m_bValid;
}

void Files11FileSystem::PrintVolumeInfo(void)
{
    if (m_bValid)
        m_HomeBlock.PrintInfo();
    else
        printf("Invalid Files11 Volume\n");
}
