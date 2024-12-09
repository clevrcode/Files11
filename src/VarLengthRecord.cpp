#include <assert.h>
#include "VarLengthRecord.h"

bool VarLengthRecord::IsVariableLengthrecordFile(std::ifstream& ifs)
{
    ifs.seekg(0, ifs.beg);
    // Assume type text
    bool typeText = true;
    uint8_t c = ifs.get();
    while (ifs.good() && typeText)
    {
        // If character is not printable and not CR and not LF and not TAB, consider the file binary
        typeText = isprint(c) || (c == 0x0D) || (c == 0x0A) || (c == 0x09);
        c = ifs.get();
    }
    return typeText;
}

int VarLengthRecord::CalculateFileLength(std::ifstream& ifs)
{
    ifs.seekg(0, ifs.beg);

    int fsize = 2;
    int line_len = 1;
    uint8_t last = 0;
    uint8_t c = ifs.get();

    while (ifs.good())
    {
        if (c == 0x0A) {
            //fsize += (last != 0x0d) ? 2 : 1;
            fsize += (line_len % 2);
            line_len = 0;
        }
        else {
            if (fsize != 2) {
                fsize += (line_len == 0) ? 2 : 0;
                line_len++;
            }
            fsize++;
        }
        last = c;
        c = ifs.get();
    }
	return fsize;
}

bool VarLengthRecord::WriteFile(std::ifstream& inFile, std::fstream& outFile, BlockList_t& blkList)
{
    char data[2][F11_BLOCK_SIZE];
    memset(data, 0, sizeof(data));
    int ptr = 0;
    std::string line;

    // Rewind the input stream
    inFile.seekg(0, inFile.beg);

    for (auto blk : blkList)
    {
        for (uint32_t lbn = blk.lbn_start; lbn <= blk.lbn_end; ++lbn)
        {
            // Read one line (up to "\n" or eof
            std::getline(inFile, line);
            while (inFile.good())
            {
                uint16_t* szPtr = (uint16_t*) & data[0][ptr];
                *szPtr = static_cast<uint16_t>(line.length());
                ptr += 2;
                if (line.length() > 0) {
                    assert(line.length() < (F11_BLOCK_SIZE + (F11_BLOCK_SIZE - ptr)));
                    memcpy(&data[0][ptr], line.c_str(), line.length());
                }
                ptr += static_cast<int>(line.length());
                ptr += ptr % 2; // align on 16 bit word
                if (ptr >= F11_BLOCK_SIZE)
                {
                    if (writeBlock(lbn, outFile, (uint8_t*)data[0]) == nullptr)
                        return false;
                    memcpy(data[0], data[1], F11_BLOCK_SIZE);
                    memset(data[1], 0, F11_BLOCK_SIZE);
                    ptr -= F11_BLOCK_SIZE;
                    break; // Get the next lbn
                }
                std::getline(inFile, line);
            }
            if (inFile.eof())
            {
                if (writeBlock(lbn, outFile, (uint8_t*)data[0]) == nullptr)
                    return false;
            }
        }
    }
    return true;
}
