#include <direct.h>
#include <assert.h>
#include "Files11FileSystem.h"
#include "Files11_defs.h"
#include "Files11Record.h"
#include "VarLengthRecord.h"
#include "FixedLengthRecord.h"
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
        FileDatabase.AppendLBN(0); // Exclude file number 0

        // Read the Home Block
        Files11Base file;
        file.ReadBlock(F11_HOME_LBN, m_dskStream);
        if (!m_HomeBlock.Initialize(file.GetHome()))
        {
			m_dskStream.close();
            return false;
        }
        file.ReadBlock(m_HomeBlock.GetBitmapSysLBN(), m_dskStream);
        if (!file.ValidateHeader())
            return false;
        // Read Storage Control Block
        Files11Base::BlockList_t blkList;
        Files11Base::GetBlockPointers(file.GetMapArea(), blkList);
        // The SCB is the first block of the Bitmap.sys file
        if (blkList.size() == 0)
            return false;

        file.ReadBlock(blkList[0].lbn_start, m_dskStream);
        if (m_HomeBlock.InitializeSCB(file.GetSCB()))
        {
            BitCounter counter;
            int vbn = 1;
            int maxFiles = m_HomeBlock.GetMaxFiles();
            for (auto lbn = m_HomeBlock.GetBitmapLBN(); lbn < m_HomeBlock.GetIndexLBN(); lbn++, vbn++)
            {
				file.ReadBlock(lbn, m_dskStream);
                int nbBits = F11_BLOCK_SIZE * 8;
                if (maxFiles < (vbn * (F11_BLOCK_SIZE * 8)))
                    nbBits = maxFiles % (F11_BLOCK_SIZE * 8);
                counter.Count(file.GetBlock(), nbBits);
            }
            //------------------------------------------
            // NOTE: The Index File Bitmap (Ref: 5.1.3)
            //       - bit 1 = header is used
            //       - bit 0 = file number is free
			//------------------------------------------
            m_HomeBlock.SetUsedHeaders(counter.GetNbHi());

			// TODO: Build link list of headers
        }
        else
            return false;

        // Build File Header Database
        const uint32_t IndexLBN = (uint32_t) m_HomeBlock.GetIndexLBN();
        FileDatabase.SetMaxFile(m_HomeBlock.GetMaxFiles());

        Files11Record IndexFileRecord;
        IndexFileRecord.Initialize(IndexLBN, m_dskStream);
        std::string filename(IndexFileRecord.GetFullName());
        if (filename != "INDEXF.SYS")
        {
            std::cerr << "Invalid File System\n";
			m_dskStream.close();
            return false;
        }
        
        FileDatabase.AppendLBN(IndexLBN); // File number 1 is the INDEXF.SYS file
        FileDatabase.Add(F11_INDEXF_SYS, IndexFileRecord);

        // Initialize the File Number to LBN index
		int ext_file_number = F11_INDEXF_SYS;
        do {
            file.ReadBlock(FileDatabase.FileNumberToLBN(ext_file_number), m_dskStream);
            F11_MapArea_t* pMap = file.GetMapArea();
            Files11Base::BlockList_t  blklist;
            ext_file_number = Files11Base::GetBlockPointers(pMap, blklist);

            if (blklist.empty()) {
	    		print_error("ERROR -- Failed to read block list for INDEXF.SYS");
                m_dskStream.close();
                return false;
            }

            for (auto& block : blklist)
            {
                for (auto lbn = block.lbn_start; lbn <= block.lbn_end; ++lbn)
                {
                    if (lbn > IndexLBN)
                    {
                        FileDatabase.AppendLBN(lbn);
                        Files11Record fileRecord;
                        int fileNumber = fileRecord.Initialize(lbn, m_dskStream);
                        if (fileNumber > 0)
                        {
							// TODO assert(fileNumber == Files11Base::FileNumberToLBN.size() - 1);
                            //printf("%06o:%06o %-20sOwner: [%03o,%03o], Protection: 0x%04x LBN: %d %c\n", fileRecord.GetFileNumber(), fileRecord.GetFileSeq(), 
                            //    fileRecord.GetFullName(), fileRecord.GetOwnerUIC() >> 8, fileRecord.GetOwnerUIC() & 0xff, fileRecord.GetFileProtection(), lbn, fileRecord.IsFileExtension() ? 'Y' : 'N');
                            FileDatabase.Add(fileNumber, fileRecord);
                        }
                    }
                }
            }

		} while (ext_file_number > 0);

        // set current working directory to the user UIC
        m_CurrentDirectory = m_HomeBlock.GetOwnerUIC();
        m_bValid = true;

		std::cout << "Number of header used: " << FileDatabase.GetNbHeaders() << std::endl;

    }
    else
    {
        print_error("ERROR -- Failed to open disk image file");
    }
	return m_bValid;
}

int Files11FileSystem::GetHighestVersion(int dirfnb, const char* filename, Files11Record &fileRecord)
{
    int highVersion = 0;

    FileList_t fileList;
    GetFileList(dirfnb, fileList);

    std::string strFileName(filename);
    for (auto& file : fileList)
    {
        Files11Record fileRec;
        if (FileDatabase.Get(file.fnumber, fileRec))
        {
            if (!fileRec.IsFileExtension() && (strFileName == fileRec.GetFullName()))
            {
                if (file.version > highVersion) {
                    highVersion = file.version;
                    fileRecord = fileRec;
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
        Files11Base::readBlock(IndexBitmapLBN + bmpIndexBlock, m_dskStream, bitmap);
        for (int filebmp = 0; (filebmp < F11_BLOCK_SIZE) && (fileNumber < maxFiles); ++filebmp)
        {
            for (int i = 0; (i < 8) && (fileNumber < maxFiles); ++i, ++fileNumber)
            {
                //int fileNumber = (bmpIndexBlock * 4096) + (filebmp * 8) + i + 1;
                int lbn = FileDatabase.FileNumberToLBN(fileNumber);
                if (lbn > 0)
                {
                    bool used = (bitmap[filebmp] & (1 << i)) != 0;
                    Files11Record fileRecord;
                    int fnb = fileRecord.Initialize(lbn, m_dskStream);
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
        Files11Base::BlockList_t blkList;
        GetBlockList(bmpRecord.GetHeaderLBN(), blkList);
        bool skipFCS = true;
        for (auto& block : blkList)
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

        Files11Base blockFile;
        int last_bitmap_block = -1;
        uint8_t* buffer = nullptr;

        for (int fnumber = 1; fnumber < m_HomeBlock.GetMaxFiles(); ++fnumber)
        {
            if (FileDatabase.Exist(fnumber))
            {
                Files11Record frec;
                FileDatabase.Get(fnumber, frec);
                if (frec.IsFileExtension())
                    continue;

                // Check if file is marked for deletion
                if (frec.IsMarkedForDeletion()) {
                    printf("FILE ID %06o,%06o . OWNER [%o,%o]\n", frec.GetFileNumber(), frec.GetFileSeq(), frec.GetOwnerUIC() >> 8, frec.GetOwnerUIC() & 0xff);
                    printf("        FILE IS MARKED FOR DELETE\n");
                }

                int vbn = 1;
                int last_vbn = frec.GetFileFCS().GetUsedBlockCount();
                int eof_bytes = frec.GetFileFCS().GetFirstFreeByte();
                Files11Base::BlockList_t fBlock;
                GetBlockList(frec.GetHeaderLBN(), blkList);              
                for (auto& block : fBlock)
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
                            buffer = blockFile.ReadBlock(bmpLBN[bitmap_block], m_dskStream);
                            last_bitmap_block = bitmap_block;
                        }
                        if (buffer) {
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
    }
    return totalErrors;
}

//
// Recursive search of directories
//

int Files11FileSystem::ValidateDirectory(const char* dirname, DirFileList_t& dirFileMap, int* pTotalFilesChecked)
{
    int totalErrors = 0;
    FileDatabase::DirList_t dirlist;
    FileDatabase.FindDirectory(dirname, dirlist);
    for (auto& dir : dirlist)
    {
        Files11Record dirRecord;
        if (FileDatabase.Get(dir.fnumber, dirRecord))
        {
            std::string strDirName;
            strDirName = FileDatabase::FormatDirectory(dirRecord.GetFileName());
            Files11Base::BlockList_t dirblks;
            GetBlockList(dirRecord.GetHeaderLBN(), dirblks);
            Files11FCS  dirFCS = dirRecord.GetFileFCS();
            int vbn = 1;
            int last_vbn = dirFCS.GetUsedBlockCount();
            int eof_bytes = dirFCS.GetFirstFreeByte();

            for (auto& block : dirblks)
            {
                for (auto lbn = block.lbn_start; (lbn <= block.lbn_end) && (vbn <= last_vbn); ++lbn, ++vbn)
                {
                    Files11Base dirFile;
                    int nbrecs = (vbn == last_vbn) ? eof_bytes : F11_BLOCK_SIZE;
                    nbrecs /= sizeof(DirectoryRecord_t);
                    DirectoryRecord_t* pRec = dirFile.ReadDirectory(lbn, m_dskStream);
                    for (int idx = 0; idx < nbrecs; idx++)
                    {
                        if (pRec[idx].fileNumber != 0)
                        {
                            DirectoryRecord_t* p = &pRec[idx];
                            (*pTotalFilesChecked)++;

                            // Check if file was found in another directory
                            auto pos = dirFileMap.find(pRec[idx].fileNumber);
                            if (pos == dirFileMap.end()) {
                                dirFileMap[pRec[idx].fileNumber] = strDirName;
                            }
                            else
                            {
                                totalErrors++;
                                std::string msg("File is present in another directory : ");
                                msg += pos->second;
                                Files11Base::PrintError(strDirName.c_str(), p, msg.c_str());
                            }

                            //std::cout << "checking file : " << name << "." << ext << ";" << pRec[idx].version << std::endl;
                            // Ignore INDEXF.SYS
                            if (p->fileNumber == F11_INDEXF_SYS)
                                continue;

                            if (p->fileRVN != 0)
                            {
                                totalErrors++;
                                Files11Base::PrintError(strDirName.c_str(), p, "RESERVED FIELD WAS NON-ZERO");
                            }
                            if (p->version > MAX_FILE_VERSION)
                            {
                                totalErrors++;
                                Files11Base::PrintError(strDirName.c_str(), p, "INVALID VERSION NUMBER");
                            }

                            Files11Record frec;
                            if (FileDatabase.Get(pRec[idx].fileNumber, frec))
                            {
                                if (frec.IsDirectory())
                                {
                                    std::string name(frec.GetFileName());
                                    if (name != "000000")
                                        totalErrors += ValidateDirectory(frec.GetFileName(), dirFileMap, pTotalFilesChecked);
                                }
                            }
                            else
                            {
                                totalErrors++;
                                Files11Base::PrintError(strDirName.c_str(), p, "FILE NOT FOUND IN FILE DATABASE");
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
        DirFileList_t DirectoryFileMap;

        if (args.size() == 2)
            args.push_back("000000");

        // Directory Validation

        int nbErrors = ValidateDirectory(args[2].c_str(), DirectoryFileMap, &totalFiles);
        // -- Find Lost Files
        // Go to all headers and check that the file belongs in a directory
        for (int fnumber = 1; fnumber < m_HomeBlock.GetMaxFiles(); ++fnumber)
        {
            if (FileDatabase.Exist(fnumber))
            {
                auto pos = DirectoryFileMap.find(fnumber);
                if (pos == DirectoryFileMap.end()) {
                    Files11Record frec;
                    FileDatabase.Get(fnumber, frec);
                    if (!frec.IsFileExtension()) {
                        nbErrors++;
                        printf("FILE ID %06o,%06o %s\n", fnumber, frec.GetFileSeq(), frec.GetFullName());
                        printf("        FILE IS NOT REACHABLE\n");
                    }
                }
            }
        }
        if (nbErrors > 0)
            std::cout << "\n -- validation found " << nbErrors << " error(s) on " << totalFiles << " files checked\n\n";
        else
            std::cout << "\n -- validation found no error on " << totalFiles << " file(s) checked\n\n";
    }
}

int Files11FileSystem::GetDirList(const char* dirname, FileDatabase::DirList_t &dlist)
{
    dlist.clear();
    if (dirname != nullptr)
    {
        FileDatabase.FindDirectory(dirname, dlist);
    }
    return static_cast<int>(dlist.size());
}

// --------------------------------------------------------------------------
// Return the list of file number present in directory specified by dirname
int Files11FileSystem::GetFileList(int dirfnb, FileList_t &fileList)
{
    Files11Record dirRecord;
    if (FileDatabase.Get(dirfnb, dirRecord))
    {
        Files11Base::BlockList_t dirblks;
        GetBlockList(dirRecord.GetHeaderLBN(), dirblks);
        Files11FCS  dirFCS = dirRecord.GetFileFCS();

        int vbn = 1;
        int last_vbn = dirFCS.GetUsedBlockCount();
        int eof_bytes = dirFCS.GetFirstFreeByte();

        for (auto& block : dirblks)
        {
            for (auto lbn = block.lbn_start; (lbn <= block.lbn_end) && (vbn <= last_vbn); ++lbn, ++vbn)
            {
                int nbrecs = (vbn == last_vbn) ? eof_bytes : F11_BLOCK_SIZE;
                nbrecs /= sizeof(DirectoryRecord_t);

                DirectoryRecord_t* pRec = (DirectoryRecord_t*)m_File.ReadBlock(lbn, m_dskStream);
                for (int idx = 0; idx < nbrecs; idx++)
                {
                    if (pRec[idx].fileNumber != 0)
                        fileList.push_back(FileInfo_t(pRec[idx].fileNumber, pRec[idx].version));
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
    Files11Base::BlockList_t blklist;
    GetBlockList(fileRec.GetHeaderLBN(), blklist);
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

        for (auto& block : blklist)
        {
            for (auto lbn = block.lbn_start; (lbn <= block.lbn_end) && (vbn <= last_vbn); ++lbn, ++vbn)
            {
                if (vbn == 1) {
                    Files11Base::readBlock(lbn, m_dskStream, buffer[0]);
                    if (vbn != last_vbn)
                        continue;
                }
                if (vbn == last_vbn)
                    EOB = (vbn == 1) ? last_block_length : (F11_BLOCK_SIZE + last_block_length);

                if (vbn > 1)
                    Files11Base::readBlock(lbn, m_dskStream, buffer[1]);

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
    Files11Base::BlockList_t blklist;
    GetBlockList(fileRec.GetHeaderLBN(), blklist);
    if (!fileFCS.IsFixedLengthRecord())
        return;

    if (!blklist.empty())
    {
        int last_vbn = fileFCS.GetUsedBlockCount();
        int high_vbn = fileFCS.GetHighVBN();
        int last_block_length = fileFCS.GetFirstFreeByte();
        int vbn = 1;

        for (auto& block : blklist)
        {
            for (auto lbn = block.lbn_start; (lbn <= block.lbn_end) && (vbn <= last_vbn); lbn++, vbn++)
            {
                Files11Base file;
                uint16_t *ptr = (uint16_t *)file.ReadBlock(lbn, m_dskStream);
                int nbBytes = F11_BLOCK_SIZE;
                if (vbn == last_vbn)
                    nbBytes = last_block_length;

                char header[128];
                snprintf(header, sizeof(header), "\n\n\n\nDump of DU0:%s;%d - File ID %d,%d,%d\n", fileRec.GetFullName(), 1, fileNumber, fileRec.GetFileSeq(), 0);
                strm << header;
                snprintf(header, sizeof(header), "                  Virtual block 0,%06d - Size %d. bytes\n\n", vbn, nbBytes);
                strm << header;

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

void Files11FileSystem::DumpLBN(const Args_t &args)
{
    assert(args[0] == "DMPLBN");

    if (args.size() >= 2)
    {
        for (auto i = 1; i < args.size(); i++)
        {
            int lbn = Files11Base::StringToInt(args[i]);
            if ((lbn < 0) || (lbn > m_HomeBlock.GetNumberOfBlocks())) {
                print_error("ERROR -- Invalid LBN");
                continue;
            }

            Files11Base file;
            uint8_t *buffer = file.ReadBlock(lbn, m_dskStream);

            std::cout << "Dump of Logical Block Number: " << lbn << std::endl << std::endl;

            int nbBytes = F11_BLOCK_SIZE;
            for (int i = 0; i < (nbBytes / 16); i++)
            {
                std::cout.width(4);
                std::cout.fill('0');
                std::cout << std::hex << std::right << (i * 16) << " | ";
                for (int j = 0; j < 16; j++) {
                    int c = buffer[(i * 16) + j];
                    std::cout.width(2);
                    std::cout.fill('0');
                    std::cout << c << " ";
                }
                std::cout << " | ";
                for (int j = 0; j < 16; j++) {
                    int c = buffer[(i * 16) + j];
                    if (isprint(c))
                        std::cout << static_cast<char>(c);
                    else
                        std::cout << ".";
                }
                std::cout << " |" << std::endl;
            }
            std::cout.fill(' ');
            std::cout << std::endl << std::endl;
        }
    }
}

void Files11FileSystem::DumpHeader(int fileNumber)
{
    // Validate file number
    if (!FileDatabase.Exist(fileNumber))
    {
        print_error("ERROR -- Invalid file number");
        return;
    }
    int lbn = FileDatabase.FileNumberToLBN(fileNumber);
    Files11Base hdrFile;
    F11_FileHeader_t *pHeader = hdrFile.ReadHeader(lbn, m_dskStream);
    Files11Record fRec;
    FileDatabase.Get(fileNumber, fRec);
    std::cout << "\n\n";
    std::cout << "Dump of DU0:" << fRec.GetFullName() << " - File ID " << std::oct << fRec.GetFileNumber() << "," << fRec.GetFileSeq() << ",0\n";
    std::cout << "                                                           File header\n\n";
    std::cout << "HEADER AREA\n";
    std::cout << "        H.IDOF                 ";
    std::cout.width(3); std::cout.fill('0');
    std::cout << std::oct << std::right << (int)pHeader->fh1_b_idoffset << std::endl;
    std::cout << "        H.MPOF                 ";
    std::cout.width(3); std::cout.fill('0');
    std::cout << std::oct << std::right << (int)pHeader->fh1_b_mpoffset << std::endl;
    std::cout << "        H.FNUM,                " << std::endl;
    std::cout << "        H.FSEQ                 (" << std::oct << pHeader->fh1_w_fid_num << "," << pHeader->fh1_w_fid_seq << ")" << std::endl;
    std::cout << "        H.FLEV,                " << pHeader->fh1_w_struclev << std::endl;
    std::cout << "        H.OWN                  [";
    std::cout.width(3); std::cout.fill('0');
    std::cout << (pHeader->fh1_w_fileowner >> 8) << ",";
    std::cout.width(3); std::cout.fill('0');
    std::cout << (pHeader->fh1_w_fileowner & 0xFF) << "]" << std::endl;
    std::cout << "        H.FPRO                 " << hdrFile.GetFileProtectionString(pHeader->fh1_w_fileprot) << std::endl;

    std::cout << "        H.UCHA                 ";
    std::cout.width(3); std::cout.fill('0');
    std::cout << std::oct << (int)pHeader->fh1_b_userchar << " = ";
    if (pHeader->fh1_b_userchar > 0)
        std::cout << std::dec << (int)pHeader->fh1_b_userchar << ".";
    std::cout << std::endl;
    std::cout << "        H.SCHA                 ";
    std::cout.width(3); std::cout.fill('0');
    std::cout << std::oct << (int)pHeader->fh1_b_syschar << " = ";
    if (pHeader->fh1_b_syschar > 0)
        std::cout << std::dec << (int)pHeader->fh1_b_syschar << ".";
    std::cout << std::endl;

    std::cout << "        H.UFAT                 " << std::endl;
    F11_UserAttrArea_t* pUser = hdrFile.GetUserAttr();
    std::cout << "                 F.RTYP        ";
    std::cout.width(3); std::cout.fill('0');
    std::cout << std::oct << (int)pUser->ufcs_rectype << " = ";
    if (pUser->ufcs_rectype & rt_fix)
        std::cout << "R.FIX ";
    if (pUser->ufcs_rectype & rt_vlr)
        std::cout << "R.VAR ";
    if (pUser->ufcs_rectype & rt_seq)
        std::cout << "R.SEQ ";
    std::cout << std::endl;

    std::cout << "                 F.RATT        ";
    std::cout.width(3); std::cout.fill('0');
    std::cout << std::oct << (int)pUser->ufcs_recattr << " = ";
    if (pUser->ufcs_recattr & ra_ftn)
        std::cout << "FD.FTN ";
    if (pUser->ufcs_recattr & ra_cr)
        std::cout << "FD.CR ";
    if (pUser->ufcs_recattr & ra_prn)
        std::cout << "FD.PRN ";
    std::cout << std::endl;

    std::cout.width(3); std::cout.fill('0');
    std::cout << "                 F.RSIZ        " << std::oct << pUser->ufcs_recsize << " = " << std::dec << pUser->ufcs_recsize << "." << std::endl;
    std::cout.width(3); std::cout.fill('0');
    std::cout << "                 F.HIBK        ";
    print_blocks((int)pUser->ufcs_highvbn_hi, (int)pUser->ufcs_highvbn_lo);
    std::cout.width(3); std::cout.fill('0');
    std::cout << "                 F.EFBK        ";
    print_blocks((int)pUser->ufcs_eofblck_hi, (int)pUser->ufcs_eofblck_lo);
    std::cout.width(3); std::cout.fill('0');
    std::cout << "                 F.FFBY        " << std::oct << pUser->ufcs_ffbyte << " = " << std::dec << pUser->ufcs_ffbyte << "." << std::endl;
    std::cout << "                 (REST)\n";
    uint16_t* p = (uint16_t*)((uint8_t*)&pHeader->fh1_w_ufat + sizeof(F11_UserAttrArea_t));
    std::cout << "                 ";
    for (int i = 0; i < 8; i++, p++)
    {
        std::cout.width(6); std::cout.fill('0');
        std::cout << std::oct << std::right << *p << " ";
    }
    std::cout << std::endl << "                 ";
    std::cout.width(6); std::cout.fill('0');
    std::cout << std::oct << std::right << *p << std::endl;

    std::cout.width(0);
    std::cout << "IDENTIFICATION AREA\n";
    std::string name, ext, misc;
    F11_IdentArea_t* pIdent = hdrFile.GetIdentArea();
    Files11Base::Radix50ToAscii(pIdent->filename, 3, name, true);
    Files11Base::Radix50ToAscii(pIdent->filetype, 1, ext, true);
    std::cout << "        I.FNAM                 " << name << std::endl;
    std::cout << "        I.FTYP                 " << ext << std::endl;
    std::cout << "        I.FVER                 " << name << "." << ext << ";" << pIdent->version << std::endl;
    std::cout << "        I.RVNO                 " << pIdent->revision << std::endl;
    Files11Base::MakeDate(pIdent->revision_date, misc, false);
    std::cout << "        I.RVDT                 " << misc << std::endl;
    Files11Base::MakeTime(pIdent->revision_time, misc);
    std::cout << "        I.RVTI                 " << misc << std::endl;
    Files11Base::MakeDate(pIdent->creation_date, misc, false);
    std::cout << "        I.CRDT                 " << misc << std::endl;
    Files11Base::MakeTime(pIdent->creation_time, misc);
    std::cout << "        I.CRTI                 " << misc << std::endl;
    Files11Base::MakeDate(pIdent->expiration_date, misc, false);
    std::cout << "        I.EXDT                 " << misc << std::endl;

    int ext_header = fileNumber;
    while (ext_header != 0)
    {
        Files11Base mapFile;
        F11_FileHeader_t *phdr = mapFile.ReadHeader(FileDatabase.FileNumberToLBN(ext_header), m_dskStream);
        F11_MapArea_t *pMap = mapFile.GetMapArea();
        print_map_area(pMap);
        std::cout << "CHECKSUM\n";
        std::cout << "        H.CKSM                 " << std::oct << pHeader->fh1_w_checksum << std::endl;
        ext_header = pMap->ext_FileNumber;
    }
    std::cout << std::endl;
    std::cout << std::endl;
    std::cout << std::dec;
    std::cout.fill(' ');
}

void Files11FileSystem::print_error(const char* msg)
{
    std::cout << "\n\x1b[31m" << msg << "\x1b[0m\n\n";
}

void Files11FileSystem::print_blocks(int blk_hi, int blk_lo)
{
    std::cout << "H:";
    std::cout.width(3); std::cout.fill('0');
    std::cout << std::right << std::oct << blk_hi;
    std::cout << " L:";
    std::cout.width(6); std::cout.fill('0');
    std::cout << std::right << blk_lo << " = " << std::dec << (int)((blk_hi * 0x10000) + blk_lo) << "." << std::endl;
}

void Files11FileSystem::print_map_area(F11_MapArea_t* pMap)
{
    std::cout.width(3); std::cout.fill('0');
    std::cout << "MAP AREA\n";
    std::cout.width(3); std::cout.fill('0');
    std::cout << "        M.ESQN                 ";
    std::cout.width(3); std::cout.fill('0');
    std::cout << std::right << (int)pMap->ext_SegNumber << std::endl;
    std::cout << "        M.ERVN                 ";
    std::cout.width(3); std::cout.fill('0');
    std::cout << std::right << (int)pMap->ext_RelVolNo << std::endl;
    std::cout << "        M.EFNU                 " << std::endl;
    std::cout << "        M.EFSQ                 (" << (int)pMap->ext_FileNumber << "," << (int)pMap->ext_FileSeqNumber << ")" << std::endl;
    std::cout << "        M.CTSZ                 ";
    std::cout.width(3); std::cout.fill('0');
    std::cout << std::right << (int)pMap->CTSZ << std::endl;
    std::cout << "        M.LBSZ                 ";
    std::cout.width(3); std::cout.fill('0');
    std::cout << std::right << (int)pMap->LBSZ << std::endl;
    std::cout << "        M.USE                  ";

    std::cout.width(3); std::cout.fill('0');
    std::cout << std::oct << std::right << (int)pMap->USE << std::dec << " = " << (int)pMap->USE << "." << std::endl;
    std::cout << "        M.MAX                  ";
    std::cout.width(3); std::cout.fill('0');
    std::cout << std::oct << std::right << (int)pMap->MAX << std::dec << " = " << (int)pMap->MAX << "." << std::endl;
    std::cout << "        M.RTRV                 " << std::endl;
    std::cout << "        SIZE    LBN            " << std::endl;
    F11_Format1_t* pFmt1 = (F11_Format1_t*)&pMap->pointers;
    for (int i = 0; i < (pMap->USE / 2); ++i) {
        std::cout << "        ";
        std::cout.width(8); std::cout.fill(' ');
        std::string misc(std::to_string(pFmt1->blk_count + 1) + ".");
        std::cout << std::left << misc;
        print_blocks((int)pFmt1->hi_lbn, (int)pFmt1->lo_lbn);
        pFmt1++;
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
    
    Files11Base::BlockList_t blklist;
    GetBlockList(fileRec.GetHeaderLBN(), blklist);

    //printf("Record Type : 0x%02x\n", fileFCS.GetRecordType());
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

        for (auto& block : blklist)
        {
            for (auto lbn = block.lbn_start; lbn <= block.lbn_end; lbn++)
            {
                // Skip first block, Storage Control Block)
                if (bFirstBlock) {
                    bFirstBlock = false;
                    continue;
                }
                vbn++;

                Files11Base bitFile;
                uint8_t* buffer = bitFile.ReadBlock(lbn, m_dskStream);
                if (buffer) {
                    int nbBits = F11_BLOCK_SIZE * 8;
                    if (totalBlocks < (vbn * (F11_BLOCK_SIZE * 8)))
                        nbBits = totalBlocks % (F11_BLOCK_SIZE * 8);
                    counter.Count(buffer, nbBits);
                }
                else
                {
                    error = true;
                    std::cerr << "Failed to read block\n";
                    break;
                }
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

void Files11FileSystem::TypeFile(int fnumber)
{
    Files11Record fileRec;
    if (FileDatabase.Get(fnumber, fileRec))
    {
        if ((fileRec.GetFileFCS().GetRecordType() & rt_vlr) && (fileRec.GetFileFCS().GetRecordAttributes() & ra_cr)) {
            std::cout << "\x1b[33m";
            PrintFile(fileRec.GetFileNumber(), std::cout);
            std::cout << "\x1b[0m";
        }
        else
            DumpFile(fileRec.GetFileNumber(), std::cout);
    }
}

void Files11FileSystem::DumpHeader(const Args_t &args)
{
    assert(args[0] == "DMPHDR");
    if (args.size() != 2)
        return;

    std::string dir(GetCurrentWorkingDirectory());
    std::string file;
    SplitFilePath(args[1], dir, file);

    if (file.empty())
        return;

    if (dir.empty())
        dir = GetCurrentWorkingDirectory();

    FileDatabase::DirList_t dirList;
    FileDatabase.FindMatchingFiles(dir.c_str(), file.c_str(), dirList, [&](int lbn, Files11Base& obj) { obj.ReadBlock(lbn, m_dskStream); });

    for (auto &dirfiles : dirList)
    {
        for (auto& fileInfo: dirfiles.fileList)
        {
            DumpHeader(fileInfo.fnumber);
        }
    }
}

void Files11FileSystem::ListFiles(const Args_t& args)
{
    assert((args.size() > 0) && ((args[0] == "CAT") || (args[0] == "TYPE")));
    std::string dir(GetCurrentWorkingDirectory());
    std::string file(ALL_FILES);

    if (args.size() == 2) {
        SplitFilePath(args[1], dir, file);
        if (dir.empty())
            dir = GetCurrentWorkingDirectory();
        if (file.empty())
            file = ALL_FILES;
    }

    std::string filename;
    std::string file_ext;
    std::string fileversion;
    FileDatabase::SplitName(file, filename, file_ext, fileversion);

    FileDatabase::DirList_t dirList;
    FileDatabase.FindMatchingFiles(dir.c_str(), file.c_str(), dirList, [&](int lbn, Files11Base& obj) { obj.ReadBlock(lbn, m_dskStream); });

    for (auto& dirRec : dirList)
    {
        if (!dirRec.fileList.empty())
        {
            // If no version is specified, only list the highest version
            int hi_version = 0;
            if (fileversion.length() == 0) {
                for (auto f : dirRec.fileList) {
                    if (f.version > hi_version)
                        hi_version = f.version;
                }
            }
            else if (fileversion != "*")
            {
                hi_version = Files11Base::StringToInt(fileversion);
            }

            for (auto f : dirRec.fileList) {
                if (hi_version == 0 || (f.version == hi_version))
                    TypeFile(f.fnumber);
            }
        }
    }
}

void Files11FileSystem::ExportFiles(const char* dirname, const char* filename, const char* outdir)
{
    if ((dirname == nullptr) || (filename == nullptr) || (outdir == nullptr))
        return;

    std::string directory(outdir);
    if (directory == ".") {
        directory += "\\";
        directory += m_HomeBlock.GetVolumeName();
    }
    _mkdir(directory.c_str());
    _chdir(directory.c_str());

    //std::string pdpdir(DirDatabase::makeKey(DirDatabase::FormatDirectory(dirname)));
    std::string pdpdir(FileDatabase::FormatDirectory(dirname));
    std::cout << "-------------------------------------------------------\n";
    std::cout << "Output directory: " << directory << std::endl;
    std::cout << pdpdir << std::endl;
    std::cout << filename << std::endl;

    FileDatabase::DirList_t dlist;
    GetDirList(pdpdir.c_str(), dlist);
    for (auto& dir : dlist)
    {
        Files11Record dirRec;
        FileDatabase.Get(dir.fnumber, dirRec);
        std::string prefix("---");
        std::cout << prefix << dirRec.GetFullName() << std::endl;

        std::vector<Files11Record> filteredList;

        FileList_t fileList;
        GetFileList(dir.fnumber, fileList);
        for (auto& file : fileList)
        {
            Files11Record frec;
            if (FileDatabase.Get(file.fnumber, frec, filename))
            {
                filteredList.push_back(frec);
                //std::string name(frec.GetFullName());
                ////if ((name.length(0 > 4)&&(name.substr(name.length()-4) == ".DIR"))
                //if (frec.IsDirectory())
                //    std::cout << "********************** DIRECTORY *************************\n";
                //std::cout << prefix << prefix << ">" << frec.GetFullName() << std::endl;
                //if (frec.IsDirectory())
                //    std::cout << "**********************************************************\n";
            }
        }
        if (!filteredList.empty())
        {
            std::cout << "------------------------------------------------------\n";
            std::cout << "Create directory: " << dirRec.GetFullName() << std::endl;
            _mkdir(dirRec.GetFileName());
            _chdir(dirRec.GetFileName());
            for (auto f : filteredList)
            {
                if (f.GetFileFCS().GetRecordType() & rt_vlr) {
                    std::fstream oStream;
                    oStream.open(f.GetFullName(), std::fstream::out | std::fstream::trunc | std::ifstream::binary);
                    if (oStream.is_open()) {
                        PrintFile(f.GetFileNumber(), oStream);
                        oStream.close();
                    }
                }
                //else
                //    WriteFile(f.GetFileNumber(), std::cout);
            }
            _chdir("..");
        }
    }
    return;
}

void Files11FileSystem::SplitFilePath(const std::string& path, std::string& dir, std::string& file)
{
    // split dir and file
    auto pos = path.find(']');
    if (pos != std::string::npos)
    {
        dir = path.substr(0, pos + 1);
        file = ((pos + 1) < path.length()) ? path.substr(pos + 1) : "*.*;*";
    }
    else
    {
        dir = "";
        file = path;
    }
}

void Files11FileSystem::ListDirs(const Args_t& args)
{
    assert((args.size() > 0) && (args[0] == "DIR"));

    std::string dir(GetCurrentWorkingDirectory());
    std::string file(ALL_FILES);

    if (args.size() == 2) {
        SplitFilePath(args[1], dir, file);
        if (dir.empty())
            dir = GetCurrentWorkingDirectory();
        if (file.empty())
            file = ALL_FILES;
    }

    FileDatabase::DirList_t dirList;
    FileDatabase.FindMatchingFiles(dir.c_str(), file.c_str(), dirList, [&](int lbn, Files11Base& obj) { obj.ReadBlock(lbn, m_dskStream); });

    // If the filtered list is not empty
    int GrandUsedBlocks = 0;
    int GrandTotalBlocks = 0;
    int GrandTotalFiles = 0;
    int DirectoryCount = 0;

    for (auto& dirRec : dirList)
    {
        if (!dirRec.fileList.empty())
        {
            int usedBlocks = 0;
            int totalBlocks = 0;
            int totalFiles = 0;
            DirectoryCount++;

            Files11Record drec;
            FileDatabase.Get(dirRec.fnumber, drec);

            std::cout << "\nDirectory DU0:" << FileDatabase::FormatDirectory(drec.GetFileName()) << std::endl;
            std::cout << m_File.GetCurrentDate() << "\n";
            for (auto& frec : dirRec.fileList)
            {
                Files11Record rec;
                Files11Base::BlockList_t blklist;
                FileDatabase.Get(frec.fnumber, rec);
                rec.PrintRecord(frec.version);
                usedBlocks += rec.GetUsedBlockCount();
                totalBlocks += GetBlockList(rec.GetHeaderLBN(), blklist);
                totalFiles++;
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
    // Grand Total
    if ((DirectoryCount > 1) && (GrandTotalFiles > 0)) {
        std::cout << "Grand total of " << GrandUsedBlocks << "./" << GrandTotalBlocks << ". blocks in " << GrandTotalFiles << ". files in " << DirectoryCount << ". directories\n";
    }
    else if (GrandTotalFiles == 0)
    {
        std::cout << "\x1b[31m" << "\nDIR -- No such file(s)\n\n" << "\x1b[0m\n";
    }
}

void Files11FileSystem::ChangeWorkingDirectory(const char* dir)
{
    std::string newdir(FileDatabase::FormatDirectory(dir));
    bool found = false;
    if (newdir.length() > 0)
    {
        // No wildcard allowed for this command
        if (newdir.find('*') == std::string::npos)
        {
            std::vector<int> dlist;
            if (FileDatabase.DirectoryExist(newdir.c_str()))
            {
                m_CurrentDirectory = newdir;
                found = true;
            }
        }
    }
    if (!found)
        print_error("ERROR -- Invalid UIC");
}

void Files11FileSystem::Close(void)
{
    if (m_dskStream.is_open()) {
        m_dskStream.close();
    }
}

void Files11FileSystem::PrintVolumeInfo(void)
{
    if (m_bValid)
        m_HomeBlock.PrintInfo();
    else
        print_error("ERROR -- Invalid Files11 Volume");
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
        Files11Base bitFile;
        int indexLBN = m_HomeBlock.GetBitmapLBN() + block;
        uint8_t *buffer = bitFile.ReadBlock(indexLBN, m_dskStream);
        // Bit 1 == used, 0 == free
        if (used)
            buffer[index] |= (1 << bit);
        else
            buffer[index] &= ~(1 << bit);
        bitFile.WriteBlock(m_dskStream);
    }
    return true;
}

//
// Mark blocks as : used if used is true, free otherwise

bool Files11FileSystem::MarkDataBlock(Files11Base::BlockList_t blkList, bool used)
{
    Files11Record bitmapRec;
    if (!FileDatabase.Get(F11_BITMAP_SYS, bitmapRec))
    {
        std::cout << "Invalid Disk Image\n";
        return false;
    }

    // Make a list of blocks (LBN) to be marked
    std::vector<int> dataBlkList;
    for (auto& blk : blkList) {
        for (auto lbn = blk.lbn_start; lbn <= blk.lbn_end; ++lbn)
            dataBlkList.push_back(lbn);
    }

    Files11Base::BlockList_t bitmapBlklist;
    GetBlockList(bitmapRec.GetHeaderLBN(), bitmapBlklist);
    if (!bitmapBlklist.empty())
    {
        bool  bFirstBlock = true;
        bool  error = false;
        int   totalBlocks = m_HomeBlock.GetNumberOfBlocks();
        int   firstBlock = 0;
        int   lastBlock = firstBlock + (F11_BLOCK_SIZE * 8);

        for (auto& blk : bitmapBlklist)
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
                    Files11Base bitFile;
                    uint8_t *buffer = bitFile.ReadBlock(lbn, m_dskStream);
                    //
                    std::vector<int> remainingBlks;
                    for (auto chkBlk : dataBlkList) {
                        if ((chkBlk >= firstBlock) && (chkBlk < lastBlock)) {
                            int vblock = chkBlk - firstBlock;
                            int index = vblock / 8;
                            int bit = vblock % 8;
                            // Bit 1 == free, 0 == used
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
                    bitFile.WriteBlock(m_dskStream);
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
        print_error("ERROR -- Directory not found");
        return false;
    }

    const int NB_DIR_ENTRY = F11_BLOCK_SIZE / sizeof(DirectoryRecord_t);
    Files11FCS fcs = dirRec.GetFileFCS();
    int lastVBN    = fcs.GetHighVBN();
    int eofBlock   = fcs.GetEOFBlock();
    int ffbyte     = fcs.GetFirstFreeByte();
    int recSize    = fcs.GetRecordSize();
    int firstFreePtr = ffbyte / recSize;

    assert(recSize == 16);

    // Go through the whole directory to find other files of the same name
    Files11Base::BlockList_t dirBlklist;
    GetBlockList(dirRec.GetHeaderLBN(), dirBlklist);

    int freeEntryLBN = 0;
    int freeEntryIDX = 0;
    int highVersion  = 0;
    int lastLBN      = 0;
    int vbn          = 1;
    Files11Base dirFile;
    for (auto& blk : dirBlklist)
    {
        for (auto lbn = blk.lbn_start; lbn <= blk.lbn_end; ++lbn, ++vbn)
        {
            lastLBN = lbn;
            int nbRec = (F11_BLOCK_SIZE / recSize);
            if (vbn == eofBlock)
                nbRec = firstFreePtr;

            DirectoryRecord_t* pEntry = dirFile.ReadDirectory(lbn, m_dskStream);
            for (int idx = 0; idx < nbRec; ++idx)
            {
                if ((freeEntryLBN == 0) && (pEntry[idx].fileNumber == 0)) {
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
        DirectoryRecord_t* pEntry = dirFile.ReadDirectory(freeEntryLBN, m_dskStream);
        pEntry[freeEntryIDX] = *pDirEntry;
        dirFile.WriteBlock(m_dskStream);
        return true;
    }
    //---------------------------------------
    // Get a copy of the directory header
    Files11Base hdrFile;
    F11_FileHeader_t    *pHeader = hdrFile.ReadHeader(dirRec.GetHeaderLBN(), m_dskStream);
    F11_UserAttrArea_t*  pUser   = hdrFile.GetUserAttr();
    F11_MapArea_t        *pMap    = hdrFile.GetMapArea();
    int                nbPointers = pMap->USE / POINTER_SIZE;

    //-----------------------------------------------------------------------------
    // append a new directory entry (there is free room in the last directory block
    if (firstFreePtr < NB_DIR_ENTRY)
    {
        // There is still room to append a directory entry at the end
        Files11Base file;
        DirectoryRecord_t* pEntry = file.ReadDirectory(lastLBN, m_dskStream);
        pEntry[firstFreePtr] = *pDirEntry;
        file.WriteBlock(m_dskStream);
        pUser->ufcs_ffbyte += recSize;
        if (pUser->ufcs_ffbyte >= F11_BLOCK_SIZE) {
            pUser->ufcs_ffbyte = 0;
            pUser->ufcs_eofblck_lo++;
            if (pUser->ufcs_eofblck_lo == 0)
                pUser->ufcs_eofblck_hi++;
        }
        hdrFile.WriteHeader(m_dskStream);
        dirRec.Refresh(m_dskStream);
        FileDatabase.Add(filenb, dirRec);
        return true;
    }

    //------------------------------------------------------
    // We need to allocate a new block for directory entries
    // TODO: TRY TO ALLOCATE A BLOCK CONTIGUOUS TO THE LAST DIRECTORY LBN

    if (nbPointers < NB_POINTERS_PER_HEADER)
    {
        // Get the number of blocks in the last pointer
        Files11Base::BlockList_t newBlkList;
        F11_Format1_t* pFmt1 = (F11_Format1_t*)&pMap->pointers;
        if (pFmt1[nbPointers - 1].blk_count < NB_POINTERS_PER_HEADER)
        {
            int nbBlocks = pFmt1[nbPointers - 1].blk_count + 2;
            int lbn = (pFmt1[nbPointers - 1].hi_lbn * 0x1000) + pFmt1[nbPointers - 1].lo_lbn;
            Files11Base::BlockPtrs_t oldPtrs(lbn, lbn + pFmt1[nbPointers - 1].blk_count);
            if (FindFreeBlocks(nbBlocks, newBlkList) <= 0)
            {
                print_error("ERROR -- Not enough free space");
                return false;
            }
            // Mark data blocks as used in BITMAP.SYS
            MarkDataBlock(newBlkList, true);
            // Transfer old content in new blocks
            auto oldlbn = oldPtrs.lbn_start;
            for (auto block : newBlkList)
            {
                for (auto lbn = block.lbn_start; (lbn <= block.lbn_end) && (oldlbn <= oldPtrs.lbn_end); ++lbn, ++oldlbn)
                {
                    Files11Base oldBlock;
                    uint8_t* p = oldBlock.ReadBlock(oldlbn, m_dskStream);
                    Files11Base::writeBlock(lbn, m_dskStream, p);
                }
            }
            // Free the old blocks
            Files11Base::BlockList_t oldBlkList;
            oldBlkList.push_back(oldPtrs);
            MarkDataBlock(oldBlkList, false);
            pFmt1[nbPointers - 1].blk_count++;
            pFmt1[nbPointers - 1].hi_lbn = newBlkList[0].lbn_start >> 16;
            pFmt1[nbPointers - 1].lo_lbn = newBlkList[0].lbn_start & 0XFFFF;
        }
        else
        {
            if (FindFreeBlocks(1, newBlkList) <= 0)
            {
                print_error("ERROR -- Not enough free space");
                return false;
            }
            // Mark data blocks as used in BITMAP.SYS
            MarkDataBlock(newBlkList, true);
            pMap->USE += POINTER_SIZE;
            pFmt1[nbPointers].blk_count = 0;
            pFmt1[nbPointers].hi_lbn = newBlkList[0].lbn_start >> 16;
            pFmt1[nbPointers].lo_lbn = newBlkList[0].lbn_start & 0XFFFF;
        }
        int blklbn = newBlkList[0].lbn_end;
        Files11Base newDir;
        DirectoryRecord_t* pDir = newDir.ReadDirectory(blklbn, m_dskStream, true);
        pDir[0] = *pDirEntry;
        newDir.WriteBlock(m_dskStream);

        // update the user attributes
        if (pUser->ufcs_ffbyte == 0)
        {
            pUser->ufcs_highvbn_lo = pUser->ufcs_eofblck_lo;
            pUser->ufcs_highvbn_hi = pUser->ufcs_eofblck_hi;
        }
        pUser->ufcs_ffbyte = recSize;
        hdrFile.WriteHeader(m_dskStream);
        dirRec.Refresh(m_dskStream);
        FileDatabase.Add(filenb, dirRec);
        return true;
    }
    // TODO: Case where the pointer area is full and we must add an extension header

    return false;
}

bool Files11FileSystem::DeleteDirectoryEntry(int filenb, std::vector<int> &fileNbToRemove)
{
    Files11Record dirRec;
    if (!FileDatabase.Get(filenb, dirRec))
    {
        print_error("ERROR -- Directory not found");
        return false;
    }

    const int NB_DIR_ENTRY = F11_BLOCK_SIZE / sizeof(DirectoryRecord_t);
    Files11FCS fcs = dirRec.GetFileFCS();
    int lastVBN = fcs.GetHighVBN();
    int eofBlock = fcs.GetEOFBlock();
    int ffbyte = fcs.GetFirstFreeByte();
    int recSize = fcs.GetRecordSize();
    int firstFreePtr = ffbyte / recSize;
    assert(recSize == 16);

    // Go through the whole directory to find other files of the same name
    Files11Base::BlockList_t dirBlklist;
    GetBlockList(dirRec.GetHeaderLBN(), dirBlklist);

    int vbn = 1;
    Files11Base dirFile;
    for (auto& blk : dirBlklist)
    {
        for (auto lbn = blk.lbn_start; lbn <= blk.lbn_end; ++lbn, ++vbn)
        {
            int nbRec = (F11_BLOCK_SIZE / recSize);
            if (vbn == eofBlock)
                nbRec = firstFreePtr;

            DirectoryRecord_t* pEntry = dirFile.ReadDirectory(lbn, m_dskStream);
            Files11Base newDir;
            DirectoryRecord_t* pNewEntry = newDir.ReadDirectory(lbn, m_dskStream, true);

            for (int idx = 0, newidx = 0; idx < nbRec; ++idx)
            {
                for (auto fnb : fileNbToRemove)
                {
                    if (pEntry[idx].fileNumber == fnb) {
                        pEntry[idx].fileNumber = 0;
                        break;
                    }
                }
                if (pEntry[idx].fileNumber != 0) {
                    pNewEntry[newidx++] = pEntry[idx];
                }
            }
            // Write back directory block
            newDir.WriteBlock(m_dskStream);
            // TODO: check if block is empty and if so, remove it


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
        print_error("ERROR -- Invalid file name");
        return false;
    }

    if (pdp11Dir == nullptr) {
        print_error("ERROR -- Invalid directory");
        return false;
    }

    FileDatabase::DirList_t dirList;
    FileDatabase.FindDirectory(pdp11Dir, dirList);
    if (dirList.size() != 1) {
        print_error("ERROR -- Invalid directory");
        return false;
    }   
    FileDatabase::DirInfo_t dirInfo(dirList[0]);

    // Determine file content type
    std::ifstream ifs(nativeName, std::ifstream::binary);
    if (!ifs.good())
    {
        std::cerr << "ERROR -- Failed to open file '" << nativeName << "'\n";
        return false;
    }
    ifs.close();

    // Get the raw size in bytes
    int dataSize = FixedLengthRecord::CalculateFileLength(nativeName);

    // Determine if text or binary content
    bool typeText = VarLengthRecord::IsVariableLengthRecordFile(nativeName);

    // From the type, determine the number of blocks required
    if (typeText) {
        dataSize = VarLengthRecord::CalculateFileLength(nativeName);
    }
    int nbBlocks = BLOCKDATASIZE(dataSize);
    int eofBytes = dataSize % F11_BLOCK_SIZE;

    // 1) Find/assign free blocks for the file content
    //    Make sure there is room on disk prior to assign a file number
    Files11Base::BlockList_t BlkList;
    if (FindFreeBlocks(nbBlocks, BlkList) <= 0)
    {
        print_error("ERROR -- Not enough free space");
        return false;
    }
    // Mark data blocks as used in BITMAP.SYS
    MarkDataBlock(BlkList, true);

    // Calculate how many header is required
    int NbContiguousBlksPerHeader = BLOCKS_PER_POINTER * NB_POINTERS_PER_HEADER;
    int nbHeaders = (nbBlocks + (NbContiguousBlksPerHeader - 1)) / NbContiguousBlksPerHeader;

    std::vector<int> hdrFileNumber;
    for (int hdr = 0; hdr < nbHeaders; ++hdr)
    {
        // 4) Find/assign a free file header/number for the file metadata
        int newFileNumber = FileDatabase.FindFirstFreeFile();
        if (newFileNumber <= 0) {
            print_error("ERROR -- File system full");
            return false;
        }
        hdrFileNumber.push_back(newFileNumber);
        // Mark header block as used in IndexBitmap
        MarkHeaderBlock(newFileNumber, true);
    }
    assert(nbHeaders == hdrFileNumber.size());
    // Convert file number to LBN
    int header_lbn = FileDatabase.FileNumberToLBN(hdrFileNumber[0]);

    // 5) Create a file header for the file metadata (set the block pointers)
    //F11_FileHeader_t *pHeader = (F11_FileHeader_t*) m_File.ReadBlock(header_lbn, m_dskStream);

    Files11Base newFile;
    F11_FileHeader_t* pHeader = newFile.ReadHeader(header_lbn, m_dskStream);
    int new_file_seq = pHeader->fh1_w_fid_seq + 1;
    newFile.ClearBlock();
    pHeader->fh1_b_idoffset  = F11_HEADER_FID_OFFSET;
    pHeader->fh1_b_mpoffset  = F11_HEADER_MAP_OFFSET;
    pHeader->fh1_w_fid_num   = hdrFileNumber[0];
    pHeader->fh1_w_fid_seq   = new_file_seq; // Increase the sequence number when file is reused (Ref: 3.1)
    pHeader->fh1_w_struclev  = 0x0101; // (Ref 3.4.1.5)
    pHeader->fh1_w_fileowner = F11_DEFAULT_FILE_OWNER;
    pHeader->fh1_w_fileprot  = m_HomeBlock.GetDefaultFileProtection(); //  F11_DEFAULT_FILE_PROTECTION; // Full access
    pHeader->fh1_b_userchar  = 0x80; // Set contiguous bit only (Ref:3.4.1.8)
    pHeader->fh1_b_syschar   = 0; // 
    pHeader->fh1_w_ufat      = 0; // 
    pHeader->fh1_w_checksum  = 0; // Checksum will be computed when the header is complete

    // Fill Ident Area
    F11_IdentArea_t* pIdent = newFile.GetIdentArea();
    // Encode file name, ext
    Files11Base::AsciiToRadix50(name.c_str(), 9, pIdent->filename);
    Files11Base::AsciiToRadix50(ext.c_str(), 3, pIdent->filetype);
    pIdent->version = 1;
    pIdent->revision = 1;
    memset(pIdent->revision_date, 0, sizeof(pIdent->revision_date));
    memset(pIdent->revision_time, 0, sizeof(pIdent->revision_time));
    Files11Base::FillDate((char*)pIdent->creation_date, (char*)pIdent->creation_time);
    memset(pIdent->expiration_date, 0, sizeof(pIdent->expiration_date));
    pIdent->reserved = 0;

    // Fill the map area
    F11_MapArea_t* pMap = newFile.GetMapArea();
    pMap->ext_RelVolNo = 0;
    pMap->CTSZ = 1;
    pMap->LBSZ = 3;
    pMap->USE = 0;
    pMap->MAX = (uint8_t)MAX_POINTERS;

    // 6) Transfer file content to the allocated blocks
    F11_UserAttrArea_t* pUserAttr = newFile.GetUserAttr();
    if (typeText) {
        if (!VarLengthRecord::WriteFile(nativeName, m_dskStream, BlkList, pUserAttr))
        {
            print_error("ERROR -- Write variable record length file");
            return false;
        }
    }
    else
    {
        if (!FixedLengthRecord::WriteFile(nativeName, m_dskStream, BlkList, pUserAttr))
        {
            print_error("ERROR -- Write fixed record length file");
            return false;
        }
    }

    // 9) Create a directory entry
    DirectoryRecord_t dirEntry;
    dirEntry.fileNumber = pHeader->fh1_w_fid_num;
    dirEntry.fileSeq = pHeader->fh1_w_fid_seq;
    dirEntry.version = 1;
    dirEntry.fileRVN = 0; // MUST BE 0
    memcpy(dirEntry.fileName, pIdent->filename, 6);
    memcpy(dirEntry.fileType, pIdent->filetype, 2);
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // Add the directory entry and update the header file version
    AddDirectoryEntry(dirInfo.fnumber, &dirEntry);
    pIdent->version = dirEntry.version;
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    // ~~~~~~~~~~~~~~~~~~~~~~~~~
    // Write all header files
    // ~~~~~~~~~~~~~~~~~~~~~~~~~
    for (int hdr = 0, blk = 0; hdr < nbHeaders; ++hdr)
    {
        header_lbn = FileDatabase.FileNumberToLBN(hdrFileNumber[hdr]);

        pMap->ext_FileNumber = ((hdr + 1) < nbHeaders) ? hdrFileNumber[hdr + 1] : 0;
        pMap->ext_FileSeqNumber = 0;
        if (pMap->ext_FileNumber != 0) {
            Files11Base file;
            F11_FileHeader_t* p = file.ReadHeader(FileDatabase.FileNumberToLBN(pMap->ext_FileNumber), m_dskStream);
            pMap->ext_FileSeqNumber = p->fh1_w_fid_seq + 1;
        }
        pMap->ext_SegNumber = hdr;

        // 7) Fill the pointers
        F11_Format1_t* Ptrs = (F11_Format1_t*)&pMap->pointers;
        for (int k = 0; (k < NB_POINTERS_PER_HEADER) && (blk < BlkList.size()); ++blk, ++k)
        {
            Files11Base::BlockPtrs_t& p = BlkList[blk];
            pMap->USE += POINTER_SIZE;
            Ptrs->blk_count = p.lbn_end - p.lbn_start;
            Ptrs->hi_lbn = p.lbn_start >> 16;
            Ptrs->lo_lbn = p.lbn_start & 0xFFFF;
            Ptrs++;
        }

        // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        // 8) Write the header, the checksum will be calculated, DO NOT WRITE IN HEADER AFTER THIS POINT!

        Files11Base::writeHeader(header_lbn, m_dskStream, pHeader);

        // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

        // 10) Add to the file database
        Files11Record fileRecord;
        int fileNumber = fileRecord.Initialize(header_lbn, m_dskStream);
        FileDatabase.Add(fileNumber, fileRecord);

        pHeader->fh1_w_fid_num = pMap->ext_FileNumber;
        pHeader->fh1_w_fid_seq = pMap->ext_FileSeqNumber;
    }
    // 11) Complete
    // Return true if successful
    return true;
}

bool Files11FileSystem::DeleteFile(const Args_t& args)
{
    assert((args.size() > 0) && ((args[0] == "DEL") || (args[0] == "RM")));

    for (auto arg : args)
    {
        std::string dir, fname;
        SplitFilePath(arg, dir, fname);

        // Validate file name name max 9 chars, extension max 3 chars
        std::string name, ext, version;
        FileDatabase::SplitName(fname, name, ext, version);
        if ((name.length() > 9) || (ext.length() > 3))
        {
            std::cerr << "ERROR -- Invalid file name [" << fname << "]\n";
            return false;
        }
        std::string fdir(dir);
        if (fdir.length() == 0) {
            fdir = GetCurrentWorkingDirectory();
        }

        FileDatabase::DirList_t dirlist;
        GetDirList(fdir.c_str(), dirlist);
        for (auto& dir : dirlist)
        {
            std::vector<int> fileNbToRemove;
            FileList_t fileList;
            GetFileList(dir.fnumber, fileList);
            for (auto file : fileList) {
                //std::cout << "DELETE FILE: [" << dirRec.GetFileName() << "]" << frec.GetFullName() << ";" << (int)frec.GetFileVersion() << " number: " << (int)frec.GetFileNumber() << std::endl;
                Files11Record fileRec;
                // TODO : NEED TO FILTER THIS LIST WITH THE FILENAME/EXT (use version -1 to get latest version)
                if (FileDatabase.Get(file.fnumber, fileRec, fname.c_str())) {
                    DeleteFile(file.fnumber);
                    fileNbToRemove.push_back(file.fnumber);
                }
            }
            DeleteDirectoryEntry(dir.fnumber, fileNbToRemove);
        }
    }
    return true;
}

bool Files11FileSystem::DeleteFile(int fileNumber)
{
    Files11Record frec;
    if (FileDatabase.Get(fileNumber, frec)) 
    {
        Files11Base::BlockList_t BlkList;
        GetBlockList(frec.GetHeaderLBN(), BlkList);
        // Deallocate data blocks
        MarkDataBlock(BlkList, false);

        // Set header file number to 0
        Files11Base file;
        int lbn = frec.GetHeaderLBN();
        do {
            F11_FileHeader_t *pHeader = file.ReadHeader(lbn, m_dskStream);
            int fNumber = pHeader->fh1_w_fid_num;
            pHeader->fh1_w_fid_num = 0;
            pHeader->fh1_b_syschar |= sc_mdl; // Mark for deletion
            F11_MapArea_t* pMap = file.GetMapArea();
            lbn = (pMap->ext_FileNumber == 0) ? 0 : FileDatabase.FileNumberToLBN(pMap->ext_FileNumber);
            file.WriteHeader(m_dskStream);
            // Deallocate the header block
            MarkHeaderBlock(fNumber, false);
            FileDatabase.Delete(fNumber);
        } while (lbn != 0);
        return true;
    }
    return false;
}

// Input number of free blocks needed
// Output: LBN of first free block

int Files11FileSystem::FindFreeBlocks(int nbBlocks, Files11Base::BlockList_t &foundBlkList)
{
    Files11Record fileRec;
    if (!FileDatabase.Get(F11_BITMAP_SYS, fileRec))
    {
        std::cout << "Invalid Disk Image\n";
        return -1;
    }

    Files11Base::BlockList_t blklist;
    GetBlockList(fileRec.GetHeaderLBN(), blklist);
    if (!blklist.empty())
    {
        int        vbn = 0;
        bool       bFirstBlock = true;
        bool       error = false;
        int        totalBlocks = m_HomeBlock.GetNumberOfBlocks();
        BitCounter counter;

        for (auto &block : blklist)
        {
            for (auto lbn = block.lbn_start; lbn <= block.lbn_end; lbn++)
            {
                // Skip first block, Storage Control Block)
                if (bFirstBlock) {
                    bFirstBlock = false;
                    continue;
                }
                vbn++;
                uint8_t buffer[F11_BLOCK_SIZE];
                if (Files11Base::readBlock(lbn, m_dskStream, buffer) == nullptr)
                {
                    error = true;
                    print_error("ERROR -- Failed to read block");
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
        Files11Base::BlockPtrs_t ptrs;
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

//--------------------------------------------------------
// Return the number of blocks and the list of blocks
int Files11FileSystem::GetBlockList(int lbn, Files11Base::BlockList_t &blklist)
{
    return BuildBlockList(lbn, blklist, m_dskStream);
}

//--------------------------------------------------------
// Build the list of blocks for a given File

int Files11FileSystem::BuildBlockList(int lbn, Files11Base::BlockList_t& blklist, std::fstream& istrm)
{
    int count = 0;
    uint8_t expected_segment = 0;
    Files11Base file;

    // Clear the plock list
    blklist.clear();

    do {
        F11_FileHeader_t *pHeader = file.ReadHeader(lbn, istrm);
        F11_MapArea_t* pMap = file.GetMapArea();
        if (pMap->ext_SegNumber != expected_segment)
        {
            fprintf(stderr, "WARNING: Invalid segment (lbn: %d): expected %d, read %d\n", lbn, expected_segment, pMap->ext_SegNumber);
        }
		expected_segment++;
        lbn = FileDatabase.FileNumberToLBN(Files11Base::GetBlockPointers(pMap, blklist));
    } while (lbn > 0);

    return Files11Base::GetBlockCount(blklist);
}
