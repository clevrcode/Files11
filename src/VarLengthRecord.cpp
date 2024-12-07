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

