#include <assert.h>
#include <time.h>
#include "Files11Base.h"

const char* Files11Base::months[] = { "JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OCT", "NOV", "DEC" };
const char* Files11Base::radix50Chars = " ABCDEFGHIJKLMNOPQRSTUVWXYZ$.%0123456789";

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
    }
}

void Files11Base::MakeUIC(uint8_t* uic, std::string& strUIC)
{
    
}

void Files11Base::PrintError(const char *dir, DirectoryRecord_t* p, const char *msg)
{
    std::string name, ext;
    Radix50ToAscii(p->fileName, 3, name, true);
    Radix50ToAscii(p->fileType, 1, ext, true);
    printf("%s FILE ID %06o,%06o,%o %s.%s;%o\n", dir, p->fileNumber, p->fileSeq, p->fileRVN, name.c_str(), ext.c_str(), p->version);
    printf(" - %s\n", msg);
}

void Files11Base::FillDate(char *pDate, char *pTime /* = nullptr */)
{
    time_t rawtime;
    time(&rawtime);

    struct tm tinfo;
    localtime_s(&tinfo, &rawtime);
    if (pDate != nullptr) {
        // output format is "DDMMMYY"
        char buffer[8];
        sprintf_s(buffer, sizeof(buffer), "%02d%3s%c%c", tinfo.tm_mday, months[tinfo.tm_mon], (tinfo.tm_year / 10) + '0', (tinfo.tm_year % 10) + '0');
        memcpy(pDate, buffer, 7);
    }

    if (pTime != nullptr) {
        // Output format is "HHMMSS"
        char buffer[8];
        snprintf(buffer, sizeof(buffer), "%02d%02d%02d", tinfo.tm_hour, tinfo.tm_min, tinfo.tm_sec);
        memcpy(pTime, buffer, 6);
    }
}

const std::string Files11Base::GetCurrentDate(void)
{
    time_t rawtime;
    struct tm tinfo;
    m_CurrentDate.clear();
    // get current timeinfo
    time(&rawtime);
    errno_t err = localtime_s(&tinfo, &rawtime);
    if (err == 0) {
        char buf[8];
        snprintf(buf, sizeof(buf), "%02d:%02d\n", tinfo.tm_hour, tinfo.tm_min);
        m_CurrentDate = std::to_string(tinfo.tm_mday) + "-" + months[tinfo.tm_mon] + "-" + std::to_string(tinfo.tm_year + 1900) + " " + buf;
    }
    return m_CurrentDate;
}

const std::string Files11Base::GetCurrentPDPTime(void)
{
    time_t rawtime;
    struct tm tinfo;
    m_CurrentTime.clear();
    // get current timeinfo
    time(&rawtime);
    errno_t err = localtime_s(&tinfo, &rawtime);
    if (err == 0) {
        char buf[16];
        snprintf(buf, sizeof(buf), "%02d:%02d:%02d ", tinfo.tm_hour, tinfo.tm_min, tinfo.tm_sec);
        //m_CurrentTime = buf;
        m_CurrentTime = buf + std::to_string(tinfo.tm_mday) + "-" + months[tinfo.tm_mon] + "-" + std::to_string(tinfo.tm_year + 1900) + "\n";
    }
    return m_CurrentTime;
}

const std::string Files11Base::GetFileProtectionString(uint16_t pro)
{
    const char* strProtection = "RWED";
    m_FileProtection.clear();
    for (int i = 0; i < 4; ++i) {
        m_FileProtection += (i > 0) ? "," : "[";
        for (int j = 0; j < 4; ++j) {
            if ((pro & 0x01) == 0)
                m_FileProtection += strProtection[j];
            pro >>= 1;
        }
    }
    m_FileProtection += "]";
    return m_FileProtection;
}

// Convert a 16-bit Radix-50 value to ASCII
void Files11Base::Radix50ToAscii(uint16_t *pR50, int len, std::string &str, bool strip) 
{
    // Radix-50 character set

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
    std::string radix50(radix50Chars);
    auto pos = radix50.find(c);
    if (pos == std::string::npos)
        return -1;
    assert((pos >= 0) && (pos < 050));
    return (int) pos;
}

// For each 3 src chars -> 1 16 bits word
void Files11Base::AsciiToRadix50(const char* src, size_t srclen, uint16_t* dest)
{
    // Radix-50 character set
    size_t len = strlen(src);
    for (auto i = 0; i < srclen; i += 3)
    {
        uint16_t tmp = 0;
        for (auto j = 0; j < 3; ++j) {
            tmp *= 050;
            char ctmp = ((i + j) < len) ? src[i + j] : ' ';
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

bool Files11Base::WriteHeader(int lbn, std::fstream& istrm, ODS1_FileHeader_t* pHeader)
{
    pHeader->fh1_w_checksum = CalcChecksum((uint16_t*)pHeader, (sizeof(ODS1_FileHeader_t) - sizeof(uint16_t)) / 2);
    return writeBlock(lbn, istrm, (uint8_t*)pHeader) != nullptr;
}

