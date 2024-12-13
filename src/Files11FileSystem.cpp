#include <regex>
#include <bitset>
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
        m_FileNumberToLBN.clear();
        m_FileNumberToLBN.push_back(0); // Exclude file number 0

        // Read the Home Block
        m_bValid = m_HomeBlock.Initialize(m_dskStream);


        // Build File Header Database
        const uint32_t IndexLBN = (uint32_t) m_HomeBlock.GetIndexLBN();
        int nbFiles = m_HomeBlock.GetMaxFiles();

        Files11Record IndexFileRecord(IndexLBN);
        IndexFileRecord.Initialize(IndexLBN, m_dskStream);
        std::string filename(IndexFileRecord.GetFullName());
        if (filename == "INDEXF.SYS")
            m_FileNumberToLBN.push_back(IndexLBN); // File number 1 is the INDEXF.SYS file
        else
        {
            std::cerr << "Invalid File System\n";
            return false;
        }
        auto BlkList = IndexFileRecord.GetBlockList();
        if (!BlkList.empty())
        {
            for (auto block : BlkList)
            {
                for (auto lbn = block.lbn_start; lbn <= block.lbn_end; ++lbn)
                {
                    if (lbn > IndexLBN)
                    {
                        m_FileNumberToLBN.push_back(lbn);
                        Files11Record fileRecord(IndexLBN);
                        int fileNumber = fileRecord.Initialize(lbn, m_dskStream);
                        if (fileNumber > 0)
                        {
                            printf("%06o:%06o %-20sOwner: [%03o,%03o], Protection: 0x%04x LBN: %d\n", fileRecord.GetFileNumber(), fileRecord.GetFileSeq(), 
                                fileRecord.GetFullName(), fileRecord.GetOwnerUIC() >> 8, fileRecord.GetOwnerUIC() & 0xff, fileRecord.GetFileProtection(), lbn);
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

const std::string Files11FileSystem::GetCurrentPDPTime(void)
{
    time_t rawtime;
    struct tm tinfo;
    m_CurrentTime.clear();
    // get current timeinfo
    time(&rawtime);
    errno_t err = localtime_s(&tinfo, &rawtime);
    if (err == 0) {
        const char* months[] = { "JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OCT", "NOV", "DEC" };
        char buf[16];
        snprintf(buf, sizeof(buf), "%02d:%02d:%02d ", tinfo.tm_hour, tinfo.tm_min, tinfo.tm_sec);
        //m_CurrentTime = buf;
        m_CurrentTime = buf + std::to_string(tinfo.tm_mday) + "-" + months[tinfo.tm_mon] + "-" + std::to_string(tinfo.tm_year + 1900) + "\n";
    }
    return m_CurrentTime;
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

int Files11FileSystem::ValidateIndexBitmap(void)
{
    const int IndexBitmapLBN = m_HomeBlock.GetBitmapLBN();
    const int nbIndexBlock = m_HomeBlock.GetIndexSize();
    const int IndexLBN = m_HomeBlock.GetIndexLBN();
    const int maxFiles = m_HomeBlock.GetMaxFiles();
    int totalErrors = 0;

    int fileNumber = 1;
    for (int bmpIndexBlock = 0; bmpIndexBlock < nbIndexBlock; ++bmpIndexBlock)
    {
        uint8_t bitmap[F11_BLOCK_SIZE];
        readBlock(IndexBitmapLBN + bmpIndexBlock, m_dskStream, bitmap);
        for (int filebmp = 0; (filebmp < F11_BLOCK_SIZE) && (fileNumber < maxFiles); ++filebmp)
        {
            for (int i = 0; (i < 8) && (fileNumber < maxFiles); ++i, ++fileNumber)
            {
                //int fileNumber = (bmpIndexBlock * 4096) + (filebmp * 8) + i + 1;               
                if (fileNumber < m_FileNumberToLBN.size())
                {
                    bool used = (bitmap[filebmp] & (1 << i)) != 0;
                    Files11Record fileRecord(IndexLBN);
                    int lbn = m_FileNumberToLBN[fileNumber];
                    int fnb = fileRecord.Initialize(lbn, m_dskStream);

                    if (fileNumber == 0127) {
                        printf("File: %s LBN: %d\n", fileRecord.GetFullName(), lbn);
                        printf("Owner: %o\n", fileRecord.GetOwnerUIC());
                        printf("Protection: 0x%04X\n", fileRecord.GetFileProtection());
                    }

                    if ((fnb > 0) && !used)
                    {
                        totalErrors++;
                        std::cout << "File number " << fileNumber << " used but bitmap bit cleared: " << fileRecord.GetFullName() << std::endl;
                    }
                    else if ((fnb == 0) && used)
                    {
                        totalErrors++;
                        std::cout << "File number " << fileNumber << " not used but bitmap bit set: " << fileRecord.GetFullName() << std::endl;
                    }
                }
            }
        }
        if (fileNumber >= maxFiles)
            break;
    }
    return totalErrors;
}

int Files11FileSystem::ValidateStorageBitmap(void)
{
    int totalErrors = 0;
    Files11Record bmpRecord;
    if (FileDatabase.Get(F11_BITMAP_SYS, bmpRecord))
    {
        std::string fname(bmpRecord.GetFullName());
        assert(fname == "BITMAP.SYS");
        std::vector<int> bmpLBN;
        BlockList_t blkList = bmpRecord.GetBlockList();
        bool skipFCS = true;
        for (auto block : blkList)
        {
            for (auto lbn = block.lbn_start; lbn <= block.lbn_end; ++lbn) {
                if (skipFCS) {
                    skipFCS = false;
                    continue;
                }
                bmpLBN.push_back(lbn);
            }
        }
        const int NumberOfBlocks = m_HomeBlock.GetNumberOfBlocks();
        assert(bmpLBN.size() >= (NumberOfBlocks / 4096));

        uint8_t buffer[F11_BLOCK_SIZE];
        int last_bitmap_block = -1;

        for (int fnumber = 1; fnumber < m_HomeBlock.GetMaxFiles(); ++fnumber)
        {
            if (FileDatabase.Exist(fnumber))
            {
                Files11Record frec;
                FileDatabase.Get(fnumber, frec);
                // Check if file is marked for deletion
                if (frec.IsMarkedForDeletion()) {
                    printf("FILE ID %06o,%06o . OWNER [%o,%o]\n", frec.GetFileNumber(), frec.GetFileSeq(), frec.GetOwnerUIC() >> 8, frec.GetOwnerUIC() & 0xff);
                    printf("        FILE IS MARKED FOR DELETE\n");
                }

                int vbn = 1;
                int last_vbn = frec.GetFileFCS().GetUsedBlockCount();
                int eof_bytes = frec.GetFileFCS().GetFirstFreeByte();
                BlockList_t fBlock = frec.GetBlockList();
                for (auto block : fBlock)
                {
                    for (auto lbn = block.lbn_start; lbn <= block.lbn_end; ++lbn)
                    {
                        //if (lbn == 0x15ed9) {
                        //    std::cout << "LBN 0x15ed9 used by " << frec.GetFullName() << "\n";
                        //}
                        int bitmap_block = lbn / 4096;
                        int bitmap_index = (lbn % 4096) / 8;
                        int bitmap_bit = lbn % 8;
                        if (bitmap_block != last_bitmap_block)
                        {
                            readBlock(bmpLBN[bitmap_block], m_dskStream, buffer);
                            last_bitmap_block = bitmap_block;
                        }
                        // Block used = 0, Block free = 1
                        if ((buffer[bitmap_index] & (1 << bitmap_bit)) != 0) {
                            totalErrors++;
                            // error, bit should be cleared since it is in use for this file
                            std::cout << "LBN " << lbn << " not marked used, (file: " << frec.GetFullName() << ")\n";
                        }
                    }
                }
            }
        }
    }
    return totalErrors;
}

int Files11FileSystem::ValidateDirectory(const char *dirname, int* pTotalFilesChecked)
{
    int totalErrors = 0;
    DirDatabase::DirList_t dirlist;
    DirDatabase.Find(dirname, dirlist);
    for (auto dir : dirlist)
    {
        Files11Record dirRecord;
        if (FileDatabase.Get(dir.fnumber, dirRecord))
        {
            std::string strDirName;
            strDirName = DirDatabase::FormatDirectory(dirRecord.GetFileName());
            BlockList_t dirblks = dirRecord.GetBlockList();
            Files11FCS  dirFCS = dirRecord.GetFileFCS();
            int vbn = 1;
            int last_vbn = dirFCS.GetUsedBlockCount();
            int eof_bytes = dirFCS.GetFirstFreeByte();

            for (auto block : dirblks)
            {
                for (auto lbn = block.lbn_start; (lbn <= block.lbn_end) && (vbn <= last_vbn); ++lbn, ++vbn)
                {
                    uint8_t data[F11_BLOCK_SIZE];
                    int nbrecs = (vbn == last_vbn) ? eof_bytes : F11_BLOCK_SIZE;
                    nbrecs /= sizeof(DirectoryRecord_t);
                    DirectoryRecord_t* pRec = (DirectoryRecord_t*)readBlock(lbn, m_dskStream, data);
                    for (int idx = 0; idx < nbrecs; idx++)
                    {
                        if (pRec[idx].fileNumber != 0)
                        {
                            DirectoryRecord_t* p = &pRec[idx];
                            std::string name, ext;
                            Radix50ToAscii(p->fileName, 3, name, true);
                            Radix50ToAscii(p->fileType, 1, ext, true);
                            (*pTotalFilesChecked)++;
                            //std::cout << "checking file : " << name << "." << ext << ";" << pRec[idx].version << std::endl;
                            // Ignore INDEXF.SYS
                            if (p->fileNumber == F11_INDEXF_SYS)
                                continue;

                            if (p->fileRVN != 0)
                            {
                                totalErrors++;
                                std::cout << strDirName << " FILE ID " << p->fileNumber << "," << p->fileSeq << "," << p->fileRVN << " " << name << "." << ext << ";" << p->version;
                                std::cout << " - RESERVED FIELD WAS NON-ZERO\n";
                            }
                            if (p->version > 1777) // TODO ???
                            {
                                totalErrors++;
                                std::cout << strDirName << " FILE ID " << p->fileNumber << "," << p->fileSeq << "," << p->fileRVN << " " << name << "." << ext << ";" << p->version;
                                std::cout << " - INVALID VERSION NUMBER\n";
                            }

                            Files11Record frec;
                            if (FileDatabase.Get(pRec[idx].fileNumber, frec))
                            {
                                if (frec.IsDirectory())
                                {
                                    std::string name(frec.GetFileName());
                                    if (name != "000000")
                                        totalErrors += ValidateDirectory(frec.GetFileName(), pTotalFilesChecked);
                                }
                            }
                            else
                            {
                                totalErrors++;
                                std::cout << strDirName << " FILE ID " << p->fileNumber << "," << p->fileSeq << "," << p->fileRVN << " " << name << "." << ext << ";" << p->version;
                                std::cout << " - FILE NOT FOUND\n";
                            }
                        }
                    }
                }
            }
        }
    }
    return totalErrors;
}

void Files11FileSystem::VerifyFileSystem(Args_t args)
{
    // Verify that each file listed in the INDEXF.SYS file is allocated
    if (args.size() == 1)
    {
        int nbErrors = ValidateIndexBitmap();
        nbErrors += ValidateStorageBitmap();
        if (nbErrors > 0) {
            std::cout << "\n -- " << nbErrors << " error(s) found\n\n";
        }
        else
            std::cout << "\n -- no error found\n\n";
    }
    else if ((args.size() >= 2) && (args[1] == "/DV"))
    {
        int totalFiles = 0;

        if (args.size() == 2)
            args.push_back("000000");

        // Directory Validation
        int nbErrors = ValidateDirectory(args[2].c_str(), &totalFiles);
        if (nbErrors > 0)
            std::cout << "\n -- validation found " << nbErrors << " error(s) on " << totalFiles << " files checked\n\n";
        else
            std::cout << "\n -- validation found no error on " << totalFiles << " file(s) checked\n\n";
    }
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
        int max_reclength = fileFCS.GetRecordSize();
        int vbn = 1;

        uint8_t buffer[2][F11_BLOCK_SIZE];
        int     idx = 0;
        int     EOB = F11_BLOCK_SIZE;

        for (auto block : blklist)
        {
            for (auto lbn = block.lbn_start; (lbn <= block.lbn_end) && (vbn <= last_vbn); ++lbn, ++vbn)
            {
                if (vbn == 1) {
                    readBlock(lbn, m_dskStream, buffer[0]);
                    if (vbn != last_vbn)
                        continue;
                }
                if (vbn == last_vbn)
                    EOB = (vbn == 1) ? last_block_length : (F11_BLOCK_SIZE + last_block_length);

                if (vbn > 1)
                    readBlock(lbn, m_dskStream, buffer[1]);

                std::string output;
                while (idx < EOB)
                {
                    uint16_t reclen = *((uint16_t*)&buffer[0][idx]);
                    assert(reclen <= max_reclength);
                    idx += 2;
                    if (reclen > 0) {
                        std::string tmp((const char*)&buffer[0][idx], reclen);
                        output.append(tmp);
                        idx += (reclen + (reclen % 2));
                    }
                    //output += "\n";
                    output += EOL;
                }
                strm << output;
                output.clear();
                if (vbn != last_vbn) {
                    memcpy(buffer[0], buffer[1], F11_BLOCK_SIZE);
                    idx -= F11_BLOCK_SIZE;
                }
            }
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
    std::string dirName = DirDatabase::FormatDirectory(dirRecord.GetFullName());
    std::string strFileName(filename);
    // If no version specified, only output the highest version
    auto pos = strFileName.find(";");
    if (pos == std::string::npos)
    {
        Files11Record fileRec;
        int highVersion = GetHighestVersion(dirName.c_str(), filename, fileRec);
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
        GetDirFileList(dirName.c_str(), fileList);
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

void Files11FileSystem::ListFiles(const Files11Record& dirRecord, const char *filename, FileList_t& fileList)
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
                        fileList.push_back(FileInfo_t(pRec[idx].fileNumber, pRec[idx].version));
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
        int GrandUsedBlocks = 0;
        int GrandTotalBlocks = 0;
        int GrandTotalFiles = 0;
        int DirectoryCount = 0;

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

                    switch (cmd)
                    {
                    case LIST:
                    {

                        FileList_t fileList;
                        ListFiles(rec, filename, fileList);

                        if (fileList.size() > 0)
                        {
                            int usedBlocks = 0;
                            int totalBlocks = 0;
                            int totalFiles = 0;
                            found = true;
                            DirectoryCount++;

                            std::cout << "\nDirectory DU0:" << DirDatabase::FormatDirectory(rec.fileName) << std::endl;
                            std::cout << GetCurrentDate() << "\n\n";
                            // List files in this directory that matches filename
                            for (auto file : fileList)
                            {
                                Files11Record frec;
                                if (FileDatabase.Get(file.fnumber, frec))
                                {
                                    std::string fileName(frec.GetFullName());
                                    fileName += ";";
                                    fileName += std::to_string(file.version);
                                    char contiguous = frec.IsContiguous() ? 'C' : ' ';
                                    std::string strBlocks;
                                    strBlocks = std::to_string(frec.GetUsedBlockCount()) + ".";
                                    printf("%-20s%-8s%c  %s\n", fileName.c_str(), strBlocks.c_str(), contiguous, frec.GetFileCreation());
                                    usedBlocks += frec.GetUsedBlockCount();
                                    totalBlocks += frec.GetBlockCount();
                                    totalFiles++;
                                }
                            }
                            std::cout << "\nTotal of " << usedBlocks << "./" << totalBlocks << ". blocks in " << totalFiles << ". file";
                            if (totalFiles > 1)
                                std::cout << "s";
                            std::cout << "\n\n";

                            GrandUsedBlocks += usedBlocks;
                            GrandTotalBlocks += totalBlocks;
                            GrandTotalFiles += totalFiles;

                        }

                    }
                    break;

                    case TYPE:
                        found = true;
                        TypeFile(rec, filename);
                        break;
                    }
                }
            }
            // Grand Total
            if ((cmd == LIST) && (DirectoryCount > 1) && (GrandTotalFiles > 0))
            {
                std::cout << "Grand total of " << GrandUsedBlocks << "./" << GrandTotalBlocks << ". blocks in " << GrandTotalFiles << ". files in " << DirectoryCount << " directories\n";
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


bool Files11FileSystem::MarkHeaderBlock(int file_number, bool used)
{
    if (file_number <= 0)
        return false;

    file_number--;
    int block = file_number / (F11_BLOCK_SIZE * 8);
    int index = (file_number % (F11_BLOCK_SIZE * 8)) / 8;
    int bit = file_number % 8;

    if (block >= 0 && block < m_HomeBlock.GetIndexSize())
    {
        int indexLBN = m_HomeBlock.GetBitmapLBN() + block;
        uint8_t buffer[F11_BLOCK_SIZE];
        readBlock(indexLBN, m_dskStream, buffer);
        if (used)
            buffer[index] |= (1 << bit);
        else
            buffer[index] &= ~(1 << bit);
        writeBlock(indexLBN, m_dskStream, buffer);
    }
    return true;
}

//
// Mark blocks as : used if used is true, free otherwise

bool Files11FileSystem::MarkDataBlock(BlockList_t blkList, bool used)
{
    Files11Record bitmapRec;
    if (!FileDatabase.Get(F11_BITMAP_SYS, bitmapRec))
    {
        std::cout << "Invalid Disk Image\n";
        return false;
    }

    // Make a list of blocks (LBN) to be marked
    std::vector<int> dataBlkList;
    for (auto blk : blkList) {
        for (auto lbn = blk.lbn_start; lbn <= blk.lbn_end; ++lbn)
            dataBlkList.push_back(lbn);
    }

    BlockList_t bitmapBlklist = bitmapRec.GetBlockList();
    if (!bitmapBlklist.empty())
    {
        bool  bFirstBlock = true;
        bool  error = false;
        int   totalBlocks = m_HomeBlock.GetNumberOfBlocks();
        int   firstBlock = 0;
        int   lastBlock = firstBlock + (F11_BLOCK_SIZE * 8);

        for (auto blk : bitmapBlklist)
        {
            for (auto lbn = blk.lbn_start; (lbn <= blk.lbn_end) && (dataBlkList.size() > 0); ++lbn)
            {
                // Skip first block, Storage Control Block)
                if (bFirstBlock) {
                    bFirstBlock = false;
                    continue;
                }

                // Check if any block to mark within the current block
                bool found = false;
                for (auto chkBlk : dataBlkList) {
                    if ((chkBlk >= firstBlock) && (chkBlk < lastBlock)) {
                        found = true;
                        break;
                    }
                }
                if (found)
                {
                    // Read the block
                    uint8_t buffer[F11_BLOCK_SIZE];
                    readBlock(lbn, m_dskStream, buffer);

                    //
                    std::vector<int> remainingBlks;
                    for (auto chkBlk : dataBlkList) {
                        if ((chkBlk >= firstBlock) && (chkBlk < lastBlock)) {
                            int vblock = chkBlk - firstBlock;
                            int index = vblock / 8;
                            int bit = vblock % 8;
                            if (used)
                                buffer[index] &= ~(1 << bit);
                            else
                                buffer[index] |= (1 << bit);
                        }
                        else
                        {
                            remainingBlks.push_back(chkBlk);
                        }
                    }
                    writeBlock(lbn, m_dskStream, buffer);
                    dataBlkList = remainingBlks;
                }
                firstBlock = lastBlock;
                lastBlock = firstBlock + (F11_BLOCK_SIZE * 8);
            }
        }
    }
    return true;
}

bool Files11FileSystem::AddDirectoryEntry(int filenb, DirectoryRecord_t* pDirEntry)
{
    Files11Record dirRec;
    if (!FileDatabase.Get(filenb, dirRec))
    {
        std::cerr << "ERROR -- Directory not found\n";
        return false;
    }

    Files11FCS fcs = dirRec.GetFileFCS();
    int lastVBN  = fcs.GetHighVBN();
    int ffbyte   = fcs.GetFirstFreeByte();
    int recSize  = fcs.GetRecordSize();
    assert(recSize == 16);

    // Go through the whole directory to find other files of the same name
    BlockList_t dirBlklist = dirRec.GetBlockList();
    if (!dirBlklist.empty())
    {
        int freeEntryLBN = 0;
        int freeEntryIDX = 0;
        int highVersion =  0;
        int lastLBN     =  0;
        int vbn = 1;
        for (auto blk : dirBlklist)
        {
            for (auto lbn = blk.lbn_start; lbn <= blk.lbn_end; ++lbn, ++vbn)
            {
                lastLBN = lbn;
                int nbRec = (F11_BLOCK_SIZE / recSize);
                if (vbn == lastVBN)
                    nbRec = ffbyte / recSize;

                uint8_t buffer[F11_BLOCK_SIZE];
                readBlock(lbn, m_dskStream, buffer);
                DirectoryRecord_t* pEntry = (DirectoryRecord_t*)buffer;
                for (int idx = 0; idx < nbRec; ++idx)
                {
                    if (pEntry[idx].fileNumber == 0) {
                        freeEntryLBN = lbn;
                        freeEntryIDX = idx;
                    }

                    if (pEntry[idx].fileType[0] == pDirEntry->fileType[0])
                    {
                        bool match = true;
                        for (int i = 0; (i < 3) && match; ++i)
                        {
                            match = pEntry[idx].fileName[i] == pDirEntry->fileName[i];
                        }
                        if (match && (pEntry[idx].version > highVersion)) {
                            highVersion = pEntry[idx].version;
                        }
                    }
                }
            }
        }
        pDirEntry->version = highVersion + 1;
        if (freeEntryLBN != 0) {
            // update the free entry with the new directory entry
            uint8_t buffer[F11_BLOCK_SIZE];
            readBlock(freeEntryLBN, m_dskStream, buffer);
            DirectoryRecord_t* pEntry = (DirectoryRecord_t*)buffer;
            pDirEntry->fileSeq = pEntry[freeEntryIDX].fileSeq + 1;
            pEntry[freeEntryIDX] = *pDirEntry;
            writeBlock(freeEntryLBN, m_dskStream, buffer);
        }
        else
        {
            // append a new directory entry
            if (ffbyte <= (F11_BLOCK_SIZE - sizeof(DirectoryRecord_t)))
            {
                uint8_t buffer[F11_BLOCK_SIZE];
                readBlock(lastLBN, m_dskStream, buffer);
                int vbn = ffbyte / (F11_BLOCK_SIZE / recSize);
                DirectoryRecord_t* pEntry = (DirectoryRecord_t*)buffer;
                pEntry[vbn] = *pDirEntry;
                pEntry[vbn].fileSeq = 1;
                pEntry[vbn].version = 1;
                writeBlock(lastLBN, m_dskStream, buffer);
            }
            else
            {
                // TODO Need to assign a new block
            }
        }
    }   
    return true;
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

bool Files11FileSystem::AddFile(const char* nativeName, const char* pdp11Dir, const char *pdp11name)
{
    // Validate file name name max 9 chars, extension max 3 chars
    std::string name, ext, version;
    FileDatabase::SplitName(pdp11name, name, ext, version);
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
    if (!ifs.good())
    {
        std::cerr << "ERROR -- Failed to open file '" << nativeName << "'\n";
        return false;
    }
    // get length of file:
    ifs.seekg(0, ifs.end);
    int dataSize = static_cast<int>(ifs.tellg());
    ifs.seekg(0, ifs.beg);
    ifs.close();

    // 1) Determine if text or binary content
    bool typeText = VarLengthRecord::IsVariableLengthRecordFile(nativeName);

    // 2) From the type, determine the number of blocks requied
    if (typeText) {
        dataSize = VarLengthRecord::CalculateFileLength(nativeName);
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
    pHeader->fh1_w_fileowner = (03 << 8) + 054; //0x0102; // TODO m_HomeBlock.GetVolumeOwner();
    pHeader->fh1_w_fileprot  = m_HomeBlock.GetDefaultFileProtection(); //  F11_DEFAULT_FILE_PROTECTION; // Full access
    pHeader->fh1_b_userchar  = 0x80; // Set contiguous bit only (Ref:3.4.1.8)
    pHeader->fh1_b_syschar   = 0; // 
    pHeader->fh1_w_ufat      = 0; // 
    memset(pHeader->fh1_b_fill_1, 0, sizeof(pHeader->fh1_b_fill_1));
    pHeader->fh1_w_checksum  = 0; // Checksum will be computed when the header is complete

    // Fill Ident Area
    F11_IdentArea_t* pIdent = (F11_IdentArea_t*)((uint16_t*)pHeader + pHeader->fh1_b_idoffset);
    // Encode file name, ext
    AsciiToRadix50(name.c_str(), 9, pIdent->filename);
    AsciiToRadix50(ext.c_str(),  3, pIdent->filetype);
    pIdent->version  = 1;
    pIdent->revision = 1;
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

    // 6) Transfer file content to the allocated blocks
    ODS1_UserAttrArea_t* pUserAttr = (ODS1_UserAttrArea_t*)&pHeader->fh1_w_ufat;
    if (!VarLengthRecord::WriteFile(nativeName, m_dskStream, BlkList, pUserAttr))
    {
        std::cerr << "ERROR -- Write variable record length file\n";
        return false;
    }
    // Mark data blocks as used in BITMAP.SYS
    MarkDataBlock(BlkList, true);

    // 7) Fill the pointers
    F11_Format1_t* Ptrs = (F11_Format1_t*)&pMap->pointers;
    for (auto p : BlkList)
    {
        pMap->USE += ((pMap->CTSZ + pMap->LBSZ) / 2);
        Ptrs->blk_count = p.lbn_end - p.lbn_start;
        Ptrs->hi_lbn = p.lbn_start >> 16;
        Ptrs->lo_lbn = p.lbn_start & 0xFFFF;
        Ptrs++;
    }

    // 8) Write the header, the checksum will be calculated, DO NOT WRITE IN HEADER AFTER THIS POINT!
    if (!WriteHeader(header_lbn, m_dskStream, pHeader))
    {
        std::cerr << "ERROR -- Failed to write file header\n";
        return false;
    }
    // Mark header block as used in IndexBitmap
    MarkHeaderBlock(newFileNumber, true);

    // 9) Create a directory entry
    DirectoryRecord_t dirEntry;
    dirEntry.fileNumber = pHeader->fh1_w_fid_num;
    dirEntry.fileSeq    = pHeader->fh1_w_fid_seq;
    dirEntry.version    = 1;
    memcpy(dirEntry.fileName, pIdent->filename, 6);
    memcpy(dirEntry.fileType, pIdent->filetype, 2);
    dirEntry.fileRVN = 0; // MUST BE 0

    AddDirectoryEntry(dirInfo.fnumber, &dirEntry);

    // 10) Add to the file database
    Files11Record fileRecord(m_HomeBlock.GetIndexLBN());
    int fileNumber = fileRecord.Initialize(header_lbn, m_dskStream);
    if (FileDatabase.Add(fileNumber, fileRecord))
    {
        // If a directory, add to the directory database (key: dir name)
        if (fileRecord.IsDirectory()) {
            DirDatabase::DirInfo_t info(fileNumber, FileNumberToLBN(fileNumber));
            DirDatabase.Add(fileRecord.fileName, info);
        }
    }

    // 11) Complete
    // Return true if successful
    return true;
}

bool Files11FileSystem::DeleteFile(const char* pdp11Dir, const char* pdp11name)
{
    return false;
}

bool Files11FileSystem::DeleteFile(int fileNumber)
{
    return false;
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
