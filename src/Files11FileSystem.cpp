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
                            //printf("%-20sOwner: 0x%04x, Protection: 0x%04x\n", fileRecord.GetFullName(), fileRecord.GetOwnerUIC(), fileRecord.GetFileProtection());
                            if (FileDatabase.Add(fileNumber, fileRecord))
                            {
                                // If a directory, add to the directory database (key: dir name)
                                if (fileRecord.IsDirectory()) {
                                    DirDatabase::DirInfo_t info(fileNumber, FileNumberToLBN(fileNumber));
                                    DirDatabase.Add(fileRecord.fileName, info);
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

int Files11FileSystem::GetHighestVersion(const char *dirname, const char* filename, Files11Record &fileRecord)
{
    int highVersion = -1;
    if (dirname != nullptr)
    {
        DirDatabase::DirList_t dlist;
        int nb_dir = DirDatabase.Find(dirname, dlist);
        // no wildcard allowed
        if (nb_dir != 1)
            return 0;

        Files11Record dirRecord;
        if (FileDatabase.Get(dlist[0].fnumber, dirRecord))
        {
            std::string strFileName(filename);
            BlockList_t dirblks = dirRecord.GetBlockList();
            Files11FCS  dirFCS  = dirRecord.GetFileFCS();

            int vbn = 1;
            int last_vbn = dirFCS.GetUsedBlockCount();
            int eof_bytes = dirFCS.GetFirstFreeByte();

            for (auto it : dirblks)
            {
                for (auto lbn = it.lbn_start; (lbn <= it.lbn_end) && (vbn <= last_vbn); ++lbn, ++vbn)
                {
                    int nbrecs = (vbn == last_vbn) ? eof_bytes : F11_BLOCK_SIZE;
                    nbrecs /= sizeof(DirectoryRecord_t);

                    DirectoryRecord_t* pRec = (DirectoryRecord_t*)ReadBlock(lbn, m_dskStream);
                    for (int idx = 0; idx < nbrecs; idx++)
                    {
                        if (pRec[idx].fileNumber != 0)
                        {
                            Files11Record fileRec;
                            if (FileDatabase.Get(pRec[idx].fileNumber, fileRec))
                            {
                                if (strFileName == fileRec.GetFullName())
                                {
                                    if (pRec[idx].version > highVersion) {
                                        highVersion = pRec[idx].version;
                                        fileRecord = fileRec;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return highVersion;
}

// -----------------------------------------------------------
// Return the list of file number in a directory
int Files11FileSystem::GetDirFileList(const char* dirname, FileList_t &fileList)
{
    fileList.clear();
    if (dirname != nullptr)
    {
        DirDatabase::DirList_t dlist;
        int nb_dir = DirDatabase.Find(dirname, dlist);
        // no wildcard allowed
        if (nb_dir != 1)
            return 0;

        Files11Record dirRecord;
        if (FileDatabase.Get(dlist[0].fnumber, dirRecord))
        {
            BlockList_t dirblks = dirRecord.GetBlockList();
            Files11FCS  dirFCS = dirRecord.GetFileFCS();

            int vbn = 1;
            int last_vbn = dirFCS.GetUsedBlockCount();
            int eof_bytes = dirFCS.GetFirstFreeByte();

            for (auto it : dirblks)
            {
                for (auto lbn = it.lbn_start; (lbn <= it.lbn_end) && (vbn <= last_vbn); ++lbn, ++vbn)
                {
                    int nbrecs = (vbn == last_vbn) ? eof_bytes : F11_BLOCK_SIZE;
                    nbrecs /= sizeof(DirectoryRecord_t);

                    DirectoryRecord_t* pRec = (DirectoryRecord_t*)ReadBlock(lbn, m_dskStream);
                    for (int idx = 0; idx < nbrecs; idx++)
                    {
                        if (pRec[idx].fileNumber != 0)
                        {
                            FileInfo_t fileInfo(pRec[idx].fileNumber, pRec[idx].version);
                            fileList.push_back(fileInfo);
                        }
                    }
                }
            }
        }
    }
    return static_cast<int>(fileList.size());
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

void Files11FileSystem::TypeFile(const Files11Record &dirRecord, const char* filename)
{
    std::string strFileName(filename);
    // If no version specified, only output the highest version
    auto pos = strFileName.find(";");
    if (pos == std::string::npos)
    {
        Files11Record fileRec;
        int highVersion = GetHighestVersion(dirRecord.GetFullName(), filename, fileRec);
        if (highVersion > 0) {
            if (fileRec.GetFileFCS().GetRecordType() & rt_vlr)
                PrintFile(fileRec.GetFileNumber(), std::cout);
            else
                DumpFile(fileRec.GetFileNumber(), std::cout);
        }
    }
    else
    {
        FileList_t fileList;
        GetDirFileList(dirRecord.GetFullName(), fileList);
        for (auto fileInfo : fileList)
        {
            Files11Record fileRec;
            if (FileDatabase.Get(fileInfo.fnumber, fileRec, fileInfo.version, filename))
            {
                if (fileRec.GetFileFCS().GetRecordType() & rt_vlr)
                    PrintFile(fileRec.GetFileNumber(), std::cout);
                else
                    DumpFile(fileRec.GetFileNumber(), std::cout);
            }
        }
    }
}

void Files11FileSystem::ListFiles(const Files11Record& dirRecord, const char *filename)
{
    int usedBlocks = 0;
    int totalBlocks = 0;
    int totalFiles = 0;
    int vbn = 1;

    BlockList_t dirblks = dirRecord.GetBlockList();
    int last_vbn = dirRecord.GetFileFCS().GetUsedBlockCount();
    int eof_bytes = dirRecord.GetFileFCS().GetFirstFreeByte();

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
        DirDatabase::DirList_t dlist;
        int nb_dir = DirDatabase.Find(cwd.c_str(), dlist);
        if (nb_dir > 0)
        {
            for (auto dir : dlist)
            {
                Files11Record rec;
                if (FileDatabase.Get(dir.fnumber, rec))
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
                        ListFiles(rec, filename);
                        break;

                    case TYPE:
                        TypeFile(rec, filename);
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
    if ((name.length() > 9) || (ext.length() > 3))
    {
        std::cerr << "ERROR -- Invalid file name\n";
        return false;
    }


    DirDatabase::DirInfo_t dirInfo;
    if (pdp11Dir != nullptr)
    {
        DirDatabase::DirList_t dirList;
        DirDatabase.Find(pdp11Dir, dirList);
        if (dirList.size() == 1)
            dirInfo = dirList[0];
    }
    if (dirInfo.fnumber == 0)
    {
        std::cerr << "ERROR -- Invalid directory\n";
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

    int MAX_Pointers = (F11_BLOCK_SIZE - (F11_HEADER_MAP_OFFSET * 2) - sizeof(uint16_t) - (sizeof(F11_MapArea_t) - sizeof(PtrsFormat_t))) / 2;
    // TODO - CHECK IF EXTENSION(S) HEADER ARE REQUIRED

    // 4) Find/assign a free file header/number for the file metadata
    int newFileNumber = FileDatabase.FindFirstFreeFile(m_HomeBlock.GetMaxFiles());
    if (newFileNumber <= 0) {
        std::cerr << "ERROR -- File system full\n";
        return false;
    }

    // Convert file number to LBN
    int header_lbn = FileNumberToLBN(newFileNumber); 

    // 5) Create a file header for the file metadata (set the block pointers)
    ODS1_FileHeader_t *pHeader = (ODS1_FileHeader_t*) ReadBlock(header_lbn, m_dskStream);
    pHeader->fh1_b_idoffset  = F11_HEADER_FID_OFFSET;
    pHeader->fh1_b_mpoffset  = F11_HEADER_MAP_OFFSET;
    pHeader->fh1_w_fid_num   = newFileNumber;
    pHeader->fh1_w_fid_seq  += 1; // Increase the sequence number when file is reused (Ref: 3.1)
    pHeader->fh1_w_struclev  = 0x0101; // (Ref 3.4.1.5)
    pHeader->fh1_w_fileowner = m_HomeBlock.GetVolumeOwner();
    pHeader->fh1_w_fileprot  = F11_DEFAULT_FILE_PROTECTION; // Full access
    pHeader->fh1_b_userchar  = 0x80; // Set contiguous bit only (Ref:3.4.1.8)
    pHeader->fh1_b_syschar   = 0; // 
    pHeader->fh1_w_ufat      = 0; // 
    memset(pHeader->fh1_b_fill_1, 0, sizeof(pHeader->fh1_b_fill_1));
    pHeader->fh1_w_checksum  = 0; // Checksum will be computed when the header is complete

    // Fill Ident Area
    F11_IdentArea_t* pIdent = (F11_IdentArea_t*)((uint16_t*)pHeader + pHeader->fh1_b_idoffset);
    // Encode file name, ext
    AsciiToRadix50(name.c_str(), 9, pIdent->filename);
    AsciiToRadix50(ext.c_str(),  3, pIdent->filename);
    pIdent->version  = 1;
    pIdent->revision = 0;
    memset(pIdent->revision_date, 0, sizeof(pIdent->revision_date));
    memset(pIdent->revision_time, 0, sizeof(pIdent->revision_time));
    FillDate((char *)pIdent->creation_date, (char *)pIdent->creation_time);
    memset(pIdent->expiration_date, 0, sizeof(pIdent->expiration_date));
    pIdent->reserved = 0;

    // Fill the map area
    F11_MapArea_t* pMap = (F11_MapArea_t*)((uint16_t*)pHeader + pHeader->fh1_b_mpoffset);
    pMap->ext_SegNumber     = 0;
    pMap->ext_RelVolNo      = 0;
    pMap->ext_FileNumber    = 0;
    pMap->ext_FileSeqNumber = 0;
    pMap->CTSZ              = 1;
    pMap->LBSZ              = 3;
    pMap->USE               = 0;
    pMap->MAX               = (uint8_t)MAX_Pointers;

    if (BlkList.size() > pMap->MAX)
    {
        std::cerr << "ERROR -- File is too large\n";
        return false;
    }

    // Fill the pointers
    F11_Format1_t* Ptrs = (F11_Format1_t*)&pMap->pointers;
    for (auto p : BlkList)
    {
        Ptrs->blk_count = p.lbn_end - p.lbn_start;
        Ptrs->hi_lbn = p.lbn_start >> 16;
        Ptrs->lo_lbn = p.lbn_start & 0xFFFF;
        Ptrs++;
    }
    if (!WriteHeader(header_lbn, m_dskStream, pHeader))
    {
        std::cerr << "ERROR -- Failed to write file header\n";
        return false;
    }

    // 6) Create a directory entry
    ReadBlock(dirInfo.lbn, m_dskStream);
        

    // 7) Transfer file content to the allocated blocks
    VarLengthRecord::WriteFile(ifs, m_dskStream, BlkList);
    
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
