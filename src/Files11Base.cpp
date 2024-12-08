#include <assert.h>
#include "Files11Base.h"

Files11Base::Files11Base()
{
    memset(m_block, 0, sizeof(m_block));
}

uint8_t* Files11Base::ReadBlock(int lbn, std::fstream& istrm)
{
    return readBlock(lbn, istrm, m_block);
}

F11_MapArea_t* Files11Base::GetMapArea(void) const
{
    F11_MapArea_t* pMap = nullptr;
    ODS1_FileHeader_t* pHeader = (ODS1_FileHeader_t*)m_block;
    return (F11_MapArea_t*)((uint16_t*)pHeader + pHeader->fh1_b_mpoffset);
}

F11_IdentArea_t* Files11Base::GetIdentArea(void) const
{
    F11_IdentArea_t* pIdent = nullptr;
    ODS1_FileHeader_t* pHeader = (ODS1_FileHeader_t*)m_block;
    return (F11_IdentArea_t*)((uint16_t*)pHeader + pHeader->fh1_b_idoffset);
}

uint16_t Files11Base::CalcChecksum(uint16_t *buffer, size_t wordCount)
{
	uint16_t checkSum = 0;
	for (size_t i = 0; i < wordCount; i++)
		checkSum += buffer[i];
	return checkSum;
}

void Files11Base::MakeString(char* str, size_t len, std::string &outstr, bool strip)
{
	char* strBuffer = new char[len + 1];
	memset(strBuffer, 0, len + 1);
	memcpy(strBuffer, str, len);
	outstr = strBuffer;
    if (strip) {
        auto pos = outstr.find_first_of(' ');
        if (pos != std::string::npos)
            outstr = outstr.substr(0,pos);
    }
    delete[] strBuffer;
}

void Files11Base::MakeDate(uint8_t* date, std::string& fdate, bool time)
{
    int d = 0;
    int b = 0;
    fdate.clear();
    fdate += date[d++]; fdate += date[d++];
    fdate += '-';
    fdate += date[d++]; fdate += date[d++]; fdate += date[d++];
    fdate += '-';
    if ((date[d] < '7') || (date[d] > '9')) {
        fdate += "20";
    }
    else {
        fdate += "19";
    }
    fdate += (date[d] > '9') ? (date[d] - ':') + '0' : date[d];
    d++;
    fdate += date[d++];

    if (time)
    {
        fdate += ' ';
        fdate += date[d++]; fdate += date[d++];
        fdate += ':';
        fdate += date[d++]; fdate += date[d++];
        //fdate += ':';
        //fdate += date[d++]; fdate += date[d++];
    }
}

void Files11Base::MakeUIC(uint8_t* uic, std::string& strUIC)
{
    
}

// Convert a 16-bit Radix-50 value to ASCII
void Files11Base::Radix50ToAscii(uint16_t *pR50, int len, std::string &str, bool strip) 
{
    // Radix-50 character set
    const char* radix50Chars = " ABCDEFGHIJKLMNOPQRSTUVWXYZ$.%0123456789";

    // Clear output string
    str.clear();

    for (int i = 0; i < len; i++)
    {
        // Extract the individual characters from the Radix-50 value
        str += radix50Chars[(pR50[i] / 050) / 050];
        str += radix50Chars[(pR50[i] / 050) % 050];
        str += radix50Chars[pR50[i] % 050];
    }
    // strip trailing whitespaces
    if (strip && (str.length() > 0))
    {
        while ((str.length() > 0) && (str.back() == ' ')) {
            str.pop_back();
        }
    }
}

int Files11Base::GetRadix50Char(char c)
{
    std::string radix50Chars(" ABCDEFGHIJKLMNOPQRSTUVWXYZ$.%0123456789");
    auto pos = radix50Chars.find(c);
    if (pos == std::string::npos)
        return -1;
    assert((pos >= 0) && (pos < 050));
    return (int) pos;
}

// For each 3 src chars -> 1 16 bits word
void Files11Base::AsciiToRadix50(const char* src, size_t srclen, uint16_t* dest)
{
    // Radix-50 character set
    std::string radix50Chars(" ABCDEFGHIJKLMNOPQRSTUVWXYZ$.%0123456789");
    for (int i = 0; i < srclen; i += 3)
    {
        uint16_t tmp = 0;
        for (int j = 0; j < 3; ++j) {
            tmp *= 050;
            char ctmp = ((i + j) < srclen) ? src[i + j] : ' ';
            tmp += GetRadix50Char(ctmp);
        }
        *dest = tmp;
        dest++;
    }
}

uint8_t *Files11Base::readBlock(int lbn, std::fstream& istrm, uint8_t* blk)
{
    int ofs = lbn * F11_BLOCK_SIZE;
    istrm.seekg(ofs, istrm.beg);
    istrm.read((char*)blk, F11_BLOCK_SIZE);
    return istrm.good() ? blk : nullptr;
}

uint8_t* Files11Base::writeBlock(int lbn, std::fstream & istrm, uint8_t * blk)
{
    int ofs = lbn * F11_BLOCK_SIZE;
    istrm.seekg(ofs, istrm.beg);
    istrm.write((char*)blk, F11_BLOCK_SIZE);
    return istrm.good() ? blk : nullptr;
}

