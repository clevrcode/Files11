#include "Files11FileSystem.h"
#include "Files11_defs.h"


// Constructor
Files11FileSystem::Files11FileSystem()
{
    m_bValid = false;
}

Files11FileSystem::~Files11FileSystem()
{
}

bool Files11FileSystem::Open(const char *dskName)
{
    std::ifstream is(dskName, std::ifstream::binary);
    if (is) {
        // Read the Home Block
        m_bValid = m_HomeBlock.Initialize(is);

        // Build File Database

        is.close();
    }
	return m_bValid;
}

int Files11FileSystem::GetDiskSize(void)
{ 
    return m_HomeBlock.GetDiskSize();
}

bool Files11FileSystem::BuildFileDatabase(void)
{
    return false;
}

void Files11FileSystem::PrintVolumeInfo(void)
{
    if (m_bValid)
        m_HomeBlock.PrintInfo();
    else
        printf("Invalid Files11 Volume\n");
}
