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

const std::string Files11FileSystem::GetCurrentDate(void)
{
    time_t rawtime;
    struct tm tinfo;
    m_CurrentDate.clear();
    // get current timeinfo
    time(&rawtime);
    errno_t err = localtime_s(&tinfo, &rawtime);
    if (err == 0) {
        const char* months[] = {"JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};
        m_CurrentDate = std::to_string(tinfo.tm_mday) + "-" + months[tinfo.tm_mon] + "-" + std::to_string(tinfo.tm_year + 1900) + " " + std::to_string(tinfo.tm_hour) + ":" + std::to_string(tinfo.tm_min);
    }
    return m_CurrentDate;
}

// Returns the directory file number (0 if not found)
int Files11FileSystem::FindDirectory(const char* dirname) const
{
    int fileNumber = 0;
    for (auto cit = FileDatabase.cbegin(); cit != FileDatabase.cend(); ++cit)
    {
        if (cit->second.IsDirectory())
        {
            std::string directory = FormatDirectory(cit->second.fileName);
            if (directory.compare(dirname) == 0)
            {
                fileNumber = cit->second.GetFileNumber();
                break;
            }
        }
    }
    return fileNumber;
}

void Files11FileSystem::TypeFile(const char* arg)
{
    



}

void Files11FileSystem::ListFiles(const BlockList_t& blks, const Files11FCS& fileFCS)
{
    int usedBlocks = 0;
    int totalBlocks = 0;
    int totalFiles = 0;
    int vbn = 1;
    int last_vbn = fileFCS.GetUsedBlockCount();
    int eof_bytes = fileFCS.GetFirstFreeByte();

    for (BlockList_t::const_iterator it = blks.cbegin(); it != blks.cend(); ++it)
    {
        for (auto lbn = it->lbn_start; (lbn <= it->lbn_end) && (vbn <= last_vbn); ++lbn, ++vbn)
        {
            int nbrecs = (vbn == last_vbn) ? eof_bytes : F11_BLOCK_SIZE;
            nbrecs /= sizeof(DirectoryRecord_t);

            DirectoryRecord_t* pRec = (DirectoryRecord_t*) ReadBlock(lbn, m_dskStream);
            for (int idx = 0; idx < nbrecs; idx++)
            {
                if (pRec[idx].fileNumber != 0)
                {
                    std::string fileName, fileExt, strBlocks;
                    Radix50ToAscii(pRec[idx].fileName, 3, fileName, true);
                    Radix50ToAscii(pRec[idx].fileType, 1, fileExt, true);
                    fileName += "." + fileExt + ";" + std::to_string(pRec[idx].version);

                    // Get other file info
                    // BATCH.BAT;1         1.      C  18-DEC-1998 02:46
                    auto it = FileDatabase.find(pRec[idx].fileNumber);
                    if (it != FileDatabase.end()) {
                        std::string strBlocks;
                        std::string creationDate(it->second.GetFileCreation());
                        char contiguous = it->second.IsContiguous() ? 'C' : ' ';
                        strBlocks = std::to_string(it->second.GetUsedBlockCount()) + ".";
                        printf("%-20s%-8s%c  %s\n", fileName.c_str(), strBlocks.c_str(), contiguous, creationDate.c_str());
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
        cwd = FormatDirectory(dirname);

    bool found = false;
    if (cwd.length() > 0)
    {
        int dirFile = FindDirectory(cwd.c_str());
        if (dirFile != 0)
        {
            std::cout << "\nDirectory DU0:" << cwd << std::endl;
            std::cout << GetCurrentDate() << "\n\n";

            ListFiles(FileDatabase[dirFile].getBlockList(), FileDatabase[dirFile].GetFileFCS());
            found = true;
        }
        if (!found)
            std::cout << "Directory '" << cwd << "' not found!\n";
    }
    if (!found)
    {
        std::cout << "DIR -- No such file(s)\n\n";
    }
}

void Files11FileSystem::ChangeWorkingDirectory(const char* dir)
{
    std::string newdir(FormatDirectory(dir));
    bool found = false;
    if (newdir.length() > 0)
    {
        for (auto cit = FileDatabase.cbegin(); cit != FileDatabase.cend(); ++cit)
        {
            if (cit->second.IsDirectory())
            {
                std::string directory = FormatDirectory(cit->second.fileName);
                if (directory.compare(newdir) == 0)
                {
                    m_CurrentDirectory = newdir;
                    found = true;
                }
            }
        }
    }
    if (!found)
        std::cout << "SET -- Invalid UIC\n";
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
