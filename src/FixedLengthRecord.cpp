#include "FixedLengthRecord.h"

int  FixedLengthRecord::CalculateFileLength(const char* fname)
{
    std::ifstream ifs(fname, std::ifstream::binary);
    if (!ifs.good())
        return -1;
    // get size of file:
    ifs.seekg(0, ifs.end);
    int dataSize = static_cast<int>(ifs.tellg());
    ifs.close();
	return dataSize;
}

bool FixedLengthRecord::IsFixedLengthRecordFile(const char* fname)
{
	return false;
}

bool FixedLengthRecord::WriteFile(const char* fname, std::fstream& outFile, BlockList_t& blkList, ODS1_UserAttrArea_t* pUserAttr)
{
    std::ifstream inFile;
    inFile.open(fname, std::ifstream::binary);
    if (inFile.good())
    {
        char data[F11_BLOCK_SIZE];
        int ptr = 0;
        int max_reclength = 0;

        pUserAttr->ufcs_eofblck_hi = 0;
        pUserAttr->ufcs_eofblck_lo = 0;
        pUserAttr->ufcs_ffbyte = -1;
        pUserAttr->ufcs_highvbn_hi = 0;
        pUserAttr->ufcs_highvbn_lo = 0;
        pUserAttr->ufcs_rectype = rt_fix;
        pUserAttr->ufcs_recattr = 0;
        pUserAttr->ufcs_recsize = F11_BLOCK_SIZE;

        uint32_t vbn = 1;
        for (auto blk : blkList)
        {
            for (uint32_t lbn = blk.lbn_start; (lbn <= blk.lbn_end) && (pUserAttr->ufcs_ffbyte < 0); ++lbn, ++vbn)
            {
                pUserAttr->ufcs_highvbn_hi = vbn >> 16;
                pUserAttr->ufcs_highvbn_lo = vbn & 0xFFFF;
                pUserAttr->ufcs_eofblck_hi = vbn >> 16;
                pUserAttr->ufcs_eofblck_lo = vbn & 0xFFFF;
                memset(data, 0, sizeof(data));
                // Read one block of binary data
                inFile.read(data, F11_BLOCK_SIZE);
                if (inFile.good())
                {
                    writeBlock(lbn, outFile, (uint8_t*)data);
                }
                else if (inFile.eof())
                {
                    if ((pUserAttr->ufcs_ffbyte = (uint16_t)inFile.gcount()) > 0)
                        writeBlock(lbn, outFile, (uint8_t*)data);
                    else 
                    {
                        pUserAttr->ufcs_eofblck_lo++;
                        if (pUserAttr->ufcs_eofblck_lo == 0)
                            pUserAttr->ufcs_eofblck_hi++;
                    }
                }
            }
        }
        if (pUserAttr->ufcs_ffbyte == F11_BLOCK_SIZE)
        {
            pUserAttr->ufcs_eofblck_lo++;
            if (pUserAttr->ufcs_eofblck_lo == 0)
                pUserAttr->ufcs_eofblck_hi++;
            pUserAttr->ufcs_ffbyte = 0;
        }
        inFile.close();
        return true;
    }
    return false;
}
