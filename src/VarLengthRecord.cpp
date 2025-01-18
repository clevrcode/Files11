#include <assert.h>
#include "VarLengthRecord.h"

bool VarLengthRecord::IsVariableLengthRecordFile(const char *fname)
{
    // Assume binary
    bool typeText = false;
    std::ifstream ifs;
    ifs.open(fname, std::ifstream::binary);
    if (ifs.good())
    {
        ifs.seekg(0, ifs.beg);
        uint8_t c = ifs.get();
        bool bText = true;
        while (ifs.good() && bText)
        {
            // If character is not printable and not CR and not LF and not TAB, consider the file binary
            bText = isprint(c) || (c == 0x0D) || (c == 0x0A) || (c == 0x09);
            c = ifs.get();
        }
        typeText = bText;
        ifs.close();
    }
    return typeText;
}

int VarLengthRecord::CalculateFileLength(const char *fname)
{
    std::ifstream ifs;
    ifs.open(fname, std::ifstream::binary);
    if (!ifs.good())
        return -1;

    int fsize = 0;
    std::string line;
    std::getline(ifs, line);
    while (ifs.good())
    {
        // strip trailing newline or carriage-return characters
        while (!line.empty() && ((line.back() == '\r') || (line.back() == '\n')))
            line.pop_back();
        int length = static_cast<int>(line.length() + 2);
        fsize += length;
        fsize += length % 2;
        // Read one line (up to "\n" or eof)
        std::getline(ifs, line);
    };
	return fsize;
}

bool VarLengthRecord::WriteFile(const char *fname, std::fstream& outFile, BlockList_t& blkList, F11_UserAttrArea_t* pUserAttr)
{
    if ((blkList.size() == 0)||(fname == nullptr)||(pUserAttr == nullptr))
        return false;

    int fileLength = CalculateFileLength(fname);
    std::ifstream inFile;
    inFile.open(fname, std::ifstream::binary);
    if (inFile.good())
    {
        char data[2][F11_BLOCK_SIZE];
        memset(data, 0, sizeof(data));
        int ptr = 0;
        int max_reclength = 0;
        std::string line;

        // Rewind the input stream
        inFile.seekg(0, inFile.beg);

        pUserAttr->ufcs_ffbyte  = fileLength % F11_BLOCK_SIZE;
        pUserAttr->ufcs_rectype = rt_vlr;
        pUserAttr->ufcs_recattr = ra_cr;
        pUserAttr->ufcs_recsize = 0;

        uint32_t vbn = 1;
        for (auto blk : blkList)
        {
            for (uint32_t lbn = blk.lbn_start; (lbn <= blk.lbn_end) && inFile.good(); ++lbn, ++vbn)
            {
                pUserAttr->ufcs_highvbn_hi = vbn >> 16;
                pUserAttr->ufcs_highvbn_lo = vbn & 0xFFFF;
                pUserAttr->ufcs_eofblck_hi = vbn >> 16;
                pUserAttr->ufcs_eofblck_lo = vbn & 0xFFFF;

                while (true)
                {
                    // Read one line (up to "\n" or eof)
                    std::getline(inFile, line);
                    // strip trailing newline or carriage-return characters
                    while (!line.empty() && ((line.back() == '\r') || (line.back() == '\n')))
                        line.pop_back();

                    uint16_t* szPtr = (uint16_t*)&data[0][ptr];
                    *szPtr = static_cast<uint16_t>(line.length());

                    // remember the largest record size
                    if (*szPtr > max_reclength)
                        max_reclength = *szPtr;

                    ptr += 2;
                    if (line.length() > 0) {
                        assert(line.length() < (F11_BLOCK_SIZE + (F11_BLOCK_SIZE - ptr)));
                        memcpy(&data[0][ptr], line.c_str(), line.length());
                    }
                    ptr += static_cast<int>(line.length());
                    ptr += ptr % 2; // align on 16 bit word
                    if ((ptr >= F11_BLOCK_SIZE)||(inFile.eof()))
                    {
                        writeBlock(lbn, outFile, (uint8_t*)data[0]);
                        if (inFile.eof() && (ptr > F11_BLOCK_SIZE)) {
                            writeBlock(lbn+1, outFile, (uint8_t*)data[1]);
                        }
                        else {
                            memcpy(data[0], data[1], F11_BLOCK_SIZE);
                            memset(data[1], 0, F11_BLOCK_SIZE);
                        }
                        ptr -= F11_BLOCK_SIZE;
                        break; // Get the next lbn
                    }
                    if (!inFile.good() || inFile.eof()) {
                        break;
                    }
                }
            }
        }
        pUserAttr->ufcs_recsize = max_reclength;
        inFile.close();
    }
    return true;
}
