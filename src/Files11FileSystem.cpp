#include <regex>
#include "Files11FileSystem.h"
#include "Files11_defs.h"
#include "Files11Record.h"
#include "VarLengthRecord.h"
#include "BitCounter.h"

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
    m_dskStream.open(dskName, std::fstream::in | std::fstream::out | std::ifstream::binary);
    if (m_dskStream.is_open()) 
    {
        // Read the Home Block
        m_bValid = m_HomeBlock.Initialize(m_dskStream);

        // Build File Header Database
        const uint32_t IndexLBN = (uint32_t) m_HomeBlock.GetIndexLBN();
        int nbFiles = m_HomeBlock.GetMaxFiles();

        Files11Record IndexFileRecord(IndexLBN);
        IndexFileRecord.Initialize(IndexLBN, m_dskStream);
        auto BlkList = IndexFileRecord.GetBlockList();
        if (!BlkList.empty())
        {
            for (auto cit = BlkList.cbegin(); cit != BlkList.cend(); ++cit)
            {
                for (auto lbn = cit->lbn_start; lbn <= cit->lbn_end; ++lbn)
                {
                    if (lbn > IndexLBN)
                    {
                        Files11Record fileRecord(IndexLBN);
                        int fileNumber = fileRecord.Initialize(lbn, m_dskStream);
                        if (fileNumber > 0)
                        {
                            if (FileDatabase.Add(fileNumber, fileRecord))
                            {
                                // If a directory, add to the directory database (key: dir name)
                                if (fileRecord.IsDirectory()) {
                                    DirDatabase.Add(fileRecord.fileName, fileNumber);
                                }
                            }
                        }
                    }
                }
            }
            // set current working directory to the user UIC
            m_CurrentDirectory = m_HomeBlock.GetOwnerUIC();
        }
        else
        {
            std::cerr << "ERROR -- Invalid INDEXF.SYS file: Block pointers empty\n";
        }
    }
    else
    {
        std::cerr << "ERROR -- Failed to open disk image file\n";
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

void Files11FileSystem::PrintFile(int fileNumber, std::ostream& strm)
{
    Files11Record fileRec;
    FileDatabase.Get(fileNumber, fileRec);
    if (fileRec.IsFileExtension())
        return;

    const Files11FCS &fileFCS = fileRec.GetFileFCS();
    BlockList_t blklist = fileRec.GetBlockList();
    if (!fileFCS.IsVarLengthRecord())
        return;
    
    if (!blklist.empty())
    {
        int last_vbn = fileFCS.GetUsedBlockCount();
        int high_vbn = fileFCS.GetHighVBN();
        int last_block_length = fileFCS.GetFirstFreeByte();
        int vbn = 1;

        std::vector<int> blocks;
        for (auto cit = blklist.cbegin(); cit != blklist.cend(); ++cit)
        {
            for (auto lbn = cit->lbn_start; (lbn <= cit->lbn_end) && (vbn <= last_vbn); lbn++, vbn++)
                blocks.push_back(lbn);
        }

        bool first_block = true;
        uint8_t buffer[2][F11_BLOCK_SIZE];
        uint8_t* blkPtr = &(buffer[1][0]);
        uint8_t* ptr = buffer[1];
        uint8_t *EOB = blkPtr + F11_BLOCK_SIZE;
        vbn = 1;
        for (auto lbn = blocks.begin(); lbn != blocks.end(); ++lbn, ++vbn)
        {
            if (first_block)
            {
                if (readBlock(*lbn, m_dskStream, buffer[1]) == nullptr)
                {
                    std::cerr << "Failed to read block\n";
                    break;
                }
                first_block = false;
            }
            else
            {
                memcpy(buffer[0], buffer[1], sizeof(buffer[0]));
                readBlock(*lbn, m_dskStream, buffer[1]);
                blkPtr -= F11_BLOCK_SIZE;
            }
            if (vbn == last_vbn)
                EOB = buffer[1] + last_block_length;

            std::string output;

            while (blkPtr < (EOB-2))
            {
                int reclen = *((uint16_t*)blkPtr);
                uint8_t* p = blkPtr + 2 + reclen + (reclen % 2);
                if (p > EOB)
                    break;
                blkPtr += 2;
                if (reclen > 0) {
                    std::string tmp((const char*)blkPtr, reclen);
                    output.append(tmp);
                    blkPtr += (reclen + (reclen % 2));
                }
                output += "\n";
            }
            strm << output;
        }
    }
}

void Files11FileSystem::DumpFile(int fileNumber, std::ostream& strm)
{
    Files11Record fileRec;
    FileDatabase.Get(fileNumber, fileRec);
    if (fileRec.IsFileExtension())
        return;

    const Files11FCS& fileFCS = fileRec.GetFileFCS();
    BlockList_t blklist = fileRec.GetBlockList();
    if (!fileFCS.IsFixedLengthRecord())
        return;

    if (!blklist.empty())
    {
        int last_vbn = fileFCS.GetUsedBlockCount();
        int high_vbn = fileFCS.GetHighVBN();
        int last_block_length = fileFCS.GetFirstFreeByte();
        int vbn = 1;

        for (auto cit = blklist.cbegin(); cit != blklist.cend(); ++cit)
        {
            for (auto lbn = cit->lbn_start; (lbn <= cit->lbn_end) && (vbn <= last_vbn); lbn++, vbn++)
            {
                uint8_t buffer[F11_BLOCK_SIZE];
                if (readBlock(lbn, m_dskStream, buffer) == nullptr)
                {
                    std::cerr << "Failed to read block\n";
                    break;
                }
                int nbBytes = F11_BLOCK_SIZE;
                if (vbn == last_vbn) {
                    nbBytes = last_block_length;
                }

                char header[128];
                snprintf(header, sizeof(header), "\n\n\n\nDump of DU0:%s;%d - File ID %d,%d,%d\n", fileRec.GetFullName(), 1, fileNumber, fileRec.GetFileSeq(), 0);
                strm << header;
                snprintf(header, sizeof(header), "                  Virtual block 0,%06d - Size %d. bytes\n\n", vbn, nbBytes);
                strm << header;

                uint16_t* ptr = (uint16_t*)buffer;
                for (int i = 0; i < (nbBytes / 16); i++)
                {
                    // 000000    054523 000000 054523 000000 054523 000000 054523 000000
                    snprintf(header, sizeof(header), "%06o   ", i * 16);
                    std::string output(header);
                    for (int j = 0; j < 8; j++, ptr++)
                    {
                        char buf[16];
                        snprintf(buf, sizeof(buf), " %06o", *ptr);
                        output += buf;
                    }
                    strm << output << std::endl;
                }
            }
        }
        strm << "\n*** EOF ***\n\n\n";
    }
}

void Files11FileSystem::PrintFreeBlocks(void)
{
    Files11Record fileRec;
    if (!FileDatabase.Get(F11_BITMAP_SYS, fileRec))
    {
        std::cout << "Invalid Disk Image\n";
        return;
    }

    const Files11FCS& fileFCS = fileRec.GetFileFCS();
    BlockList_t blklist = fileRec.GetBlockList();

    printf("Record Type : 0x%02x\n", fileFCS.GetRecordType());
    //if (fileFCS.GetRecordType() != 0)
    //    return;

    if (!blklist.empty())
    {
        int vbn = 0;
        bool bFirstBlock = true;
        bool error = false;
        int totalBlocks = m_HomeBlock.GetNumberOfBlocks();
        int largestContiguousFreeBlock = 0;
        BitCounter counter;

        for (auto cit = blklist.cbegin(); cit != blklist.cend(); ++cit)
        {
            for (auto lbn = cit->lbn_start; lbn <= cit->lbn_end; lbn++)
            {
                // Skip first block, Storage Control Block)
                if (bFirstBlock) {
                    bFirstBlock = false;
                    continue;
                }
                vbn++;
                uint8_t buffer[F11_BLOCK_SIZE];
                if (readBlock(lbn, m_dskStream, buffer) == nullptr)
                {
                    error = true;
                    std::cerr << "Failed to read block\n";
                    break;
                }
                int nbBits = F11_BLOCK_SIZE * 8;
                if (totalBlocks < (vbn * (F11_BLOCK_SIZE * 8))) {
                    nbBits = totalBlocks % (F11_BLOCK_SIZE * 8);
                }
                counter.Count(buffer, nbBits);
            }
        }
        if (!error)
        {
            // Write report
            // DU0: has 327878. blocks free, 287122. blocks used out of 615000.
            // Largest contiguous space = 277326. blocks
            // 22025. file headers are free, 7975. headers used out of 30000.
            std::cout << "DU0: has " << counter.GetNbHi() << ". blocks free, " << counter.GetNbLo() << ". blocks used out of " << totalBlocks << ".\n";
            std::cout << "Largest contiguous space = " << counter.GetLargestContiguousHi() << ". blocks\n";
            std::cout << m_HomeBlock.GetFreeHeaders() << ". file headers are free, " << m_HomeBlock.GetUsedHeaders() << ". headers used out of " << m_HomeBlock.GetMaxFiles() << ".\n\n";
        }
    }
}

void Files11FileSystem::TypeFile(const BlockList_t& dirblks, const Files11FCS& dirFCS, const char* filename)
{
   
    // If no version is specified, only process the last version
    std::string strFileName(filename);
    int highestVersion = -1;
    int highFileNumber = -1;
    uint8_t highRecordType = 0;

    auto pos = strFileName.find(";");
    if (pos == std::string::npos)
        highestVersion = 0;

    int vbn = 1;
    int last_vbn = dirFCS.GetUsedBlockCount();
    int eof_bytes = dirFCS.GetFirstFreeByte();

    for (auto it = dirblks.cbegin(); it != dirblks.cend(); ++it)
    {
        for (auto lbn = it->lbn_start; (lbn <= it->lbn_end) && (vbn <= last_vbn); ++lbn, ++vbn)
        {
            int nbrecs = (vbn == last_vbn) ? eof_bytes : F11_BLOCK_SIZE;
            nbrecs /= sizeof(DirectoryRecord_t);

            DirectoryRecord_t* pRec = (DirectoryRecord_t*)ReadBlock(lbn, m_dskStream);
            for (int idx = 0; idx < nbrecs; idx++)
            {
                if (pRec[idx].fileNumber != 0)
                {
                    // Always process the highest version of a file, unless the version is specified
                    Files11Record fileRec;
                    if (FileDatabase.Get(pRec[idx].fileNumber, fileRec, pRec[idx].version, filename))
                    {
                        if (highestVersion >= 0)
                        {
                            if (pRec[idx].version > highestVersion) {
                                highestVersion = pRec[idx].version;
                                highFileNumber = pRec[idx].fileNumber;
                                highRecordType = fileRec.GetFileFCS().GetRecordType();
                            }
                        }
                        else
                        {
                            if (fileRec.GetFileFCS().IsVarLengthRecord())
                                PrintFile(pRec[idx].fileNumber, std::cout);
                            else
                                DumpFile(pRec[idx].fileNumber, std::cout);
                        }
                    }
                }
            }
        }
    }
    // If we only process the highest version, do it here...
    if (highestVersion > 0)
    {
        if (highRecordType & rt_vlr)
            PrintFile(highFileNumber, std::cout);
        else
            DumpFile(highFileNumber, std::cout);
    }
}

void Files11FileSystem::ListFiles(const BlockList_t& dirblks, const Files11FCS& dirFCS, const char *filename)
{
    int usedBlocks = 0;
    int totalBlocks = 0;
    int totalFiles = 0;
    int vbn = 1;
    int last_vbn = dirFCS.GetUsedBlockCount();
    int eof_bytes = dirFCS.GetFirstFreeByte();

    for (const BlockPtrs_t & cit : dirblks)
    {
        for (auto lbn = cit.lbn_start; (lbn <= cit.lbn_end) && (vbn <= last_vbn); ++lbn, ++vbn)
        {
            int nbrecs = (vbn == last_vbn) ? eof_bytes : F11_BLOCK_SIZE;
            nbrecs /= sizeof(DirectoryRecord_t);

            DirectoryRecord_t* pRec = (DirectoryRecord_t*) ReadBlock(lbn, m_dskStream);
            for (int idx = 0; idx < nbrecs; idx++)
            {
                if (pRec[idx].fileNumber != 0)
                {
                    // Get other file info
                    // BATCH.BAT;1         1.      C  18-DEC-1998 02:46
                    Files11Record fileRec;
                    if (FileDatabase.Get(pRec[idx].fileNumber, fileRec, pRec[idx].version, filename))
                    {
                        std::string fileName(fileRec.GetFullName());
                        fileName += ";";
                        fileName += std::to_string(pRec[idx].version);
                        char contiguous = fileRec.IsContiguous() ? 'C' : ' ';
                        std::string strBlocks;
                        strBlocks = std::to_string(fileRec.GetUsedBlockCount()) + ".";
                        printf("%-20s%-8s%c  %s\n", fileName.c_str(), strBlocks.c_str(), contiguous, fileRec.GetFileCreation());
                        usedBlocks += fileRec.GetUsedBlockCount();
                        totalBlocks += fileRec.GetBlockCount();
                        totalFiles++;
                    }
                    else
                    {
                        //std::cerr << "ERROR: missing file #" << std::to_string(pRec[idx].fileNumber) << std::endl;
                        continue;
                    }
                }
            }           
        }
    }
    std::cout << "\nTotal of " << usedBlocks << "./" << totalBlocks << ". blocks in " << totalFiles << ". file";
    if (totalFiles > 1)
        std::cout << "s";
    std::cout << "\n\n";
}

void Files11FileSystem::ListDirs(Cmds_e cmd, const char *dirname, const char *filename)
{
    std::string cwd;
    if ((dirname == nullptr) || (dirname[0] == '\0'))
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
                    if (rec.IsFileExtension())
                        continue;

                    found = true;
                    switch (cmd)
                    {
                    case LIST:
                        std::cout << "\nDirectory DU0:" << DirDatabase::FormatDirectory(rec.fileName) << std::endl;
                        std::cout << GetCurrentDate() << "\n\n";
                        // List files in this directory that matches filename
                        ListFiles(rec.GetBlockList(), rec.GetFileFCS(), filename);
                        break;

                    case TYPE:
                        TypeFile(rec.GetBlockList(), rec.GetFileFCS(), filename);
                        break;

                    case EXPORT:
                        break;

                    case IMPORT:
                        break;
                    }
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

//========================================================
//
// Add a new file in the PDP-11 file system
//
// 1) Determine if text or binary content
// 2) From the type, determine the number of blocks requied
// 3) Find/assign free blocks for the file content
// 4) Find/assign a free file header for the file metadata
// 5) Create a file header for the file metadata (set the block pointers)
// 6) Create a directory entry
// 7) Transfer file content to the allocated blocks
// 8) Complete

bool Files11FileSystem::AddFile(const char* nativeName, const char* pdp11Dir, const char* pdp1Name)
{
    // Validate file name name max 9 chars, extension max 3 chars
    std::string name, ext, version;
    FileDatabase::SplitName(nativeName, name, ext, version);
    if ((name.length() > 9) || (ext.length() > 9))
    {
        std::cerr << "ERROR -- Invalid file name\n";
        return false;
    }

    // 1) Determine content type
    std::ifstream ifs;
    ifs.open(nativeName, std::ifstream::binary);
    // get length of file:
    ifs.seekg(0, ifs.end);
    int dataSize = static_cast<int>(ifs.tellg());
    ifs.seekg(0, ifs.beg);

    // 1) Determine if text or binary content
    bool typeText = VarLengthRecord::IsVariableLengthrecordFile(ifs);

    // 2) From the type, determine the number of blocks requied
    if (typeText) {
        dataSize = VarLengthRecord::CalculateFileLength(ifs);
    }
    int nbBlocks = (dataSize / F11_BLOCK_SIZE) + 1;
    int eofBytes = dataSize % F11_BLOCK_SIZE;

    // 3) Find/assign free blocks for the file content
    //    Make sure there is room on disk prior to assign a file number
    BlockList_t BlkList;
    if (FindFreeBlocks(nbBlocks, BlkList) <= 0)
    {
        std::cerr << "ERROR -- Not enough free space\n";
        return false;
    }

    // 4) Find/assign a free file header/number for the file metadata
    int newFileNumber = FileDatabase.FindFirstFreeFile(m_HomeBlock.GetMaxFiles());
    if (newFileNumber <= 0) {
        std::cerr << "ERROR -- File system full\n";
        return false;
    }

    // Convert file number to LBN
    int lbn = FileNumberToLBN(newFileNumber);

    // 5) Create a file header for the file metadata (set the block pointers)
    ODS1_FileHeader_t *pHeader = (ODS1_FileHeader_t*) ReadBlock(lbn, m_dskStream);
    pHeader->fh1_b_idoffset  = 0x17;
    pHeader->fh1_b_mpoffset  = 0x2e;
    pHeader->fh1_w_fid_num   = newFileNumber;
    pHeader->fh1_w_fid_seq  += 1; // Increase the sequence number when file is reused (Ref: 3.1)
    pHeader->fh1_w_struclev  = 0x0101; // (Ref 3.4.1.5)
    pHeader->fh1_w_fileowner = 0; // TODO
    pHeader->fh1_w_fileprot  = 0; // TODO
    pHeader->fh1_b_userchar  = 0; // TODO
    pHeader->fh1_b_syschar   = 0; // TODO
    pHeader->fh1_w_ufat      = 0; // TODO

    // Fill Ident Area
    F11_IdentArea_t* pIdent = (F11_IdentArea_t*)((uint16_t*)pHeader + pHeader->fh1_b_idoffset);
    // Encode file name, ext
    AsciiToRadix50(name.c_str(), 9, pIdent->filename);
    AsciiToRadix50(ext.c_str(),  3, pIdent->filename);
    pIdent->version = 0;
    pIdent->revision = 0;
    //pIdent->revision_date[7];
    //pIdent->revision_time[6];
    //pIdent->creation_date[7];
    //pIdent->creation_time[6];
    //pIdent->expiration_date[7];
    //pIdent->reserved;
    //pIdent->ident_size[46];

    // 6) Create a directory entry
    
    // 7) Transfer file content to the allocated blocks
    
    // 8) Complete


    // Return true if successful
    return true;
}

// Input number of free blocks needed
// Output: LBN of first free block

int Files11FileSystem::FindFreeBlocks(int nbBlocks, BlockList_t &foundBlkList)
{
    Files11Record fileRec;
    if (!FileDatabase.Get(F11_BITMAP_SYS, fileRec))
    {
        std::cout << "Invalid Disk Image\n";
        return -1;
    }

    BlockList_t blklist = fileRec.GetBlockList();
    if (!blklist.empty())
    {
        int        vbn = 0;
        bool       bFirstBlock = true;
        bool       error = false;
        int        totalBlocks = m_HomeBlock.GetNumberOfBlocks();
        BitCounter counter;

        for (auto cit = blklist.cbegin(); cit != blklist.cend(); ++cit)
        {
            for (auto lbn = cit->lbn_start; lbn <= cit->lbn_end; lbn++)
            {
                // Skip first block, Storage Control Block)
                if (bFirstBlock) {
                    bFirstBlock = false;
                    continue;
                }
                vbn++;
                uint8_t buffer[F11_BLOCK_SIZE];
                if (readBlock(lbn, m_dskStream, buffer) == nullptr)
                {
                    error = true;
                    std::cerr << "Failed to read block\n";
                    break;
                }
                int nbBits = F11_BLOCK_SIZE * 8;
                if (totalBlocks < (vbn * (F11_BLOCK_SIZE * 8))) {
                    nbBits = totalBlocks % (F11_BLOCK_SIZE * 8);
                }
                counter.FindSmallestBlock(buffer, nbBits, nbBlocks);
            }
        }
        // 
        int firstFreeBlock = counter.GetSmallestBlockHi();
        if (firstFreeBlock < 0)
        {
            return -1;
        }
        BlockPtrs_t ptrs;
        while (nbBlocks > 0) {
            int nb = nbBlocks / 256;
            ptrs.lbn_start = firstFreeBlock;
            if (nb >= 1)
                ptrs.lbn_end = firstFreeBlock + 255;
            else
                ptrs.lbn_end = firstFreeBlock + (nbBlocks - 1);
            foundBlkList.push_back(ptrs);
            nbBlocks -= 256;
        }
    }
    return (int)foundBlkList.size();
}
