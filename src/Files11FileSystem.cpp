#include "Files11FileSystem.h"
#include "Files11_defs.h"
#include "Files11Record.h"

// Constructor
Files11FileSystem::Files11FileSystem()
{
    m_bValid = false;
}

Files11FileSystem::~Files11FileSystem()
{
    Close();
}

bool Files11FileSystem::Open(const char *dskName)
{
    m_DiskFileName = dskName;
    m_dskStream.open(dskName, std::ifstream::binary);
    if (m_dskStream.is_open()) {
        // Read the Home Block
        m_bValid = m_HomeBlock.Initialize(m_dskStream);

        // Build File Header Database
        int firstLBN = m_HomeBlock.GetIndexLBN();
        int nbFiles = m_HomeBlock.GetMaxFiles();
        m_CurrentDirectory = m_HomeBlock.GetOwnerUIC();
        for (int i= 0; i < nbFiles; i++)
        {
            int lbn = firstLBN + i;
            Files11Record record;
            int fileNumber = record.Initialize(lbn, m_dskStream);
            if (fileNumber > 0)
            {
                //FileDatabase_t::iterator it = FileDatabase.find(fileNumber);
                auto it = FileDatabase.find(fileNumber);
                if (it != FileDatabase.end()) {
                    std::cerr << "File number [" << fileNumber << "] duplicated\n";
                }
                else
                {
                    // Add a new entry for this file
                    FileDatabase[fileNumber] = record;
                }
            }
        }
    }
    else
    {
        std::cerr << "Failed to open disk image file\n";
    }
	return m_bValid;
}

void Files11FileSystem::ListFiles(const BlockList_t& blks, const char *creationDate, int nbBlocks)
{
    int usedBlocks = 0;
    int totalBlocks = 0;
    int totalFiles = 0;

    for (BlockList_t::const_iterator it = blks.cbegin(); it != blks.cend(); ++it)
    {
        //TODO ??? DO NOT GO OVER EOFVBN ???
        for (auto lbn = it->lbn_start; lbn <= it->lbn_end; ++lbn)
        {
            DirectoryRecord_t* pRec = (DirectoryRecord_t*) ReadBlock(lbn, m_dskStream);
            for (int idx = 0; idx < (F11_BLOCK_SIZE / sizeof(DirectoryRecord_t)); idx++)
            {
                if (pRec[idx].fileNumber != 0)
                {
                    std::string fileName, fileExt, strBlocks;
                    Radix50ToAscii(pRec[idx].fileName, 3, fileName, true);
                    Radix50ToAscii(pRec[idx].fileType, 1, fileExt, true);
                    fileName += "." + fileExt + ";" + std::to_string(pRec[idx].version);

                    // Get other file info
                    auto it = FileDatabase.find(pRec[idx].fileNumber);
                    if (it != FileDatabase.end()) {
                        std::string strBlocks;
                        std::string creationDate(it->second.GetFileCreation());
                        strBlocks = std::to_string(it->second.GetUsedBlockCount()) + ".";
                        printf("%-20s%-8s%s\n", fileName.c_str(), strBlocks.c_str(), creationDate.c_str());
                        usedBlocks += it->second.GetUsedBlockCount();
                        totalBlocks += it->second.GetBlockCount();
                        totalFiles++;
                    }
                    else
                    {
                        std::cerr << "ERROR: missing file #" << std::to_string(pRec[idx].fileNumber) << " " << fileName << std::endl;
                        continue;
                    }
                }
            }           
        }
    }
    std::cout << "\nTotal of " << usedBlocks << "./" << totalBlocks << ". blocks in " << totalFiles << ". files\n\n";

}

void Files11FileSystem::ListDirs(const char *dirname)
{
    std::string cwd;
    if (dirname == NULL)
        cwd = m_CurrentDirectory;
    else
        cwd = dirname;

    bool found = false;
    for (FileDatabase_t::const_iterator it = FileDatabase.cbegin(); it != FileDatabase.cend(); ++it)
    {
        if (it->second.IsDirectory())
        {
            std::string directory = FormatDirectory(it->second.fileName);
            if (directory.compare(cwd) == 0)
            {
                std::cout << "\nDirectory DU0:" << directory << std::endl;
                std::cout << "CURRENT TIME TODO\n\n";

                ListFiles(it->second.getBlockList(), it->second.GetFileCreation(), it->second.GetBlockCount());
                found = true;
                break;
            }
        }
    }
    if (!found)
        std::cout << "Directory '" << cwd << "' not found!\n";
}

void Files11FileSystem::Close(void)
{
    if (m_dskStream.is_open()) {
        m_dskStream.close();
    }
}

int Files11FileSystem::GetDiskSize(void)
{ 
    return m_HomeBlock.GetDiskSize();
}

bool Files11FileSystem::BuildHeaderDatabase(void)
{
    return false;
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
