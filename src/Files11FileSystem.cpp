#include <regex>
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
                if (FileDatabase.Add(fileNumber, record))
                {
                    // If a directory, add to the directory database (key: dir name)
                    if (record.IsDirectory()) {
                        DirDatabase.Add(record.fileName, fileNumber);
                    }
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
        char buf[8];
        snprintf(buf, sizeof(buf), "%02d:%02d\n", tinfo.tm_hour, tinfo.tm_min); 
        m_CurrentDate = std::to_string(tinfo.tm_mday) + "-" + months[tinfo.tm_mon] + "-" + std::to_string(tinfo.tm_year + 1900) + " " + buf;
    }
    return m_CurrentDate;
}

#if 0
int Files11FileSystem::FindFile(const char* path_name, std::vector<int> &flist) const
{
    if (path_name && (strlen(path_name) > 0))
    {
        std::string dir(m_CurrentDirectory);
        std::string fileName(path_name);
        // Check if path is specified
        if (fileName.front() == '[') {
            // Yes
            auto pos = fileName.find(']');
            if (pos != std::string::npos) {
                dir = DirDatabase::FormatDirectory(fileName.substr(0, pos + 1));
                fileName = fileName.substr(pos);
            }
        }
        // split name and extension
        if (fileName.length() > 0)
        {
        }
    }
    return (int)flist.size();
}
#endif

void Files11FileSystem::TypeFile(const char* arg)
{
#if 0
    if (!blklist.empty())
    {
        int last_block = blklist.back().lba_end;
        int last_vbn = (pFCS->ufcs_eofblck_hi << 16) + pFCS->ufcs_eofblck_lo;
        int high_vbn = (pFCS->ufcs_highvbn_hi << 16) + pFCS->ufcs_highvbn_lo;
        int last_block_length = ((last_vbn > high_vbn) && (pFCS->ufcs_ffbyte == 0)) ? BLOCK_SIZE : pFCS->ufcs_ffbyte;
        int data_written = 0;
        Files11_Record_t Record;
        size_t rec_index = 0;

        int x = 0;
        uint8_t* pLine = NULL;
        int vbn = 0;
        for (BlockList_t::iterator it = blklist.begin(); it != blklist.end(); ++it)
        {
            t_lba start = it->lba_start;
            t_lba end = it->lba_end;
            for (t_lba _lba = start; (_lba <= end) && (vbn <= last_vbn); _lba++, vbn++)
            {
                t_seccnt sects_read = 0;
                uint8_t buffer[BLOCK_SIZE];
                if ((_DEC_rdsect(uptr, _lba, buffer, &sects_read, 1, 0)) || (sects_read != 1)) {
                    fprintf(stderr, "Failed to read sector %d\n", _lba);
                    fclose(fd);
                    return false;
                }

                // number of valid bytes in this block
                size_t nbbytes = ((_lba == last_block) || (vbn == last_vbn)) ? last_block_length : BLOCK_SIZE;
                uint8_t* pbuf = buffer;
                uint8_t* plast = (buffer + nbbytes);

                for (;;)
                {
                    size_t last_recidx = rec_index;
                    void* p = ((uint8_t*)&Record) + rec_index;
                    size_t bytes2read = (plast < (pbuf + rec_index)) ? nbbytes : (plast - (pbuf + rec_index));
                    assert((bytes2read >= 0) && (bytes2read <= nbbytes));
                    if (bytes2read == 0)
                        break;
                    memcpy(p, pbuf, bytes2read);
                    rec_index = rec_index + bytes2read;
                    if (rec_index < sizeof(uint16_t))
                        break; // get another block
                    if (rec_index < (Record.rec_length + 2))
                        break; // get another block

                    // If record length > 0x0100 (256 char) consider bad record
                    if (Record.rec_length >= 0x0100)
                    {
                        fclose(fd);
                        return false;
                    }
                    // We have a complete record, 
                    // write the record to file
                    if (Record.rec_length > 0) {
                        WriteBufferToFile(fd, (void*)Record.data, Record.rec_length, true);
                        data_written += Record.rec_length;
                    }

                    pbuf += (Record.rec_length + 2) - last_recidx;
                    rec_index = 0;

                    if (_DEBUG)
                        memset(Record.data, 0, sizeof(Record.data));

                    // If record length is odd, align on even boundary
                    if (Record.rec_length & 0x01)
                        pbuf++;

                }
                if (data_written == 0) {
                    fclose(fd);
                    return false;
                }

                if (Record.rec_length == 0xffff) {
                    fclose(fd);
                    return true;
                }
            }
        }
    }
#endif    
}

void Files11FileSystem::ListFiles(const BlockList_t& blks, const Files11FCS& fileFCS)
{
#if 0
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
#endif
}

void Files11FileSystem::ListDirs(const char *dirname)
{
    std::string cwd;
    if (dirname == NULL)
        cwd = m_CurrentDirectory;
    else
        cwd = DirDatabase::FormatDirectory(dirname);

    bool found = false;
    if (cwd.length() > 0)
    {
        DirList_t dlist;
        int nb_dir = DirDatabase.Find(cwd.c_str(), dlist);
        if (nb_dir > 0)
        {
            for (auto cit = dlist.cbegin(); cit != dlist.cend(); ++cit)
            {
                Files11Record rec;
                if (FileDatabase.Get(*cit, rec))
                {
                    found = true;
                    std::cout << "\nDirectory DU0:" << DirDatabase::FormatDirectory(rec.fileName) << std::endl;
                    std::cout << GetCurrentDate() << "\n\n";

                    //ListFiles(FileDatabase[dirFile].GetBlockList(), FileDatabase[dirFile].GetFileFCS());
                }
            }
        }
        if (!found)
            std::cout << "DIR -- No such file(s)\n\n";
    }
}

void Files11FileSystem::ChangeWorkingDirectory(const char* dir)
{
    std::string newdir(DirDatabase::FormatDirectory(dir));
    bool found = false;
    if (newdir.length() > 0)
    {
        // No wildcard allowed for this command
        auto pos = newdir.find('*');
        if (pos == std::string::npos)
        {
            std::vector<int> dlist;
            if (DirDatabase.Exist(newdir.c_str()))
            {
                m_CurrentDirectory = newdir;
                found = true;
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
