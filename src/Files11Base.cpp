#include <assert.h>
#include <time.h>
#include <direct.h> // _getcwd
#include <stdlib.h> // free, perror#include <regex>
#include "Files11Base.h"

const char* Files11Base::months[] = { "JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OCT", "NOV", "DEC" };
const char* Files11Base::radix50Chars = " ABCDEFGHIJKLMNOPQRSTUVWXYZ$.%0123456789";

Files11Base::Files11Base()
{
    m_LastBlockRead = -1;
    ClearBlock();
}

void Files11Base::ClearBlock(void) 
{
    memset(m_block, 0, sizeof(m_block));
}

uint8_t* Files11Base::ReadBlock(int lbn, std::fstream& istrm)
{
    m_LastBlockRead = lbn;
    return readBlock(lbn, istrm, m_block);
}

bool Files11Base::WriteBlock(std::fstream& istrm)
{
    if (m_LastBlockRead >= 0)
        return writeBlock(m_LastBlockRead, istrm, m_block);
    return false;
}

bool Files11Base::WriteHeader(std::fstream& istrm)
{
    if (m_LastBlockRead >= 0)
        return writeHeader(m_LastBlockRead, istrm, (F11_FileHeader_t*)m_block);
    return false;
}

F11_FileHeader_t* Files11Base::ReadHeader(int lbn, std::fstream& istrm, bool clear/*=false*/)
{
    m_LastBlockRead = lbn;
    uint8_t* p = readBlock(lbn, istrm, m_block);
    if (clear) {
        memset(p, 0, sizeof(m_block));
    }
    return (F11_FileHeader_t*)p;
}

bool Files11Base::ValidateHeader(F11_FileHeader_t* pHeader /*=nullptr*/) const
{
	F11_FileHeader_t* p = (pHeader == nullptr) ? (F11_FileHeader_t*)m_block : pHeader;
    return validateHeader(p);
}

DirectoryRecord_t* Files11Base::ReadDirectory(int lbn, std::fstream& istrm, bool clear/*=false*/)
{
    m_LastBlockRead = lbn;
    uint8_t *p = readBlock(lbn, istrm, m_block);
    if (clear) {
        memset(p, 0, sizeof(m_block));
    }
    return (DirectoryRecord_t*)p;
}

F11_FileHeader_t* Files11Base::GetHeader(void* ptr/*=nullptr*/) const
{
    return (ptr == nullptr) ? (F11_FileHeader_t*)m_block : (F11_FileHeader_t*)ptr;
}

F11_HomeBlock_t* Files11Base::GetHome(void* p /*=nullptr*/) const
{
    return (p == nullptr) ? (F11_HomeBlock_t*)m_block : (F11_HomeBlock_t*)p;
}

F11_MapArea_t* Files11Base::GetMapArea(F11_FileHeader_t* ptr /*=nullptr*/) const
{
    F11_FileHeader_t* pHeader = (ptr == nullptr) ? (F11_FileHeader_t*)m_block : ptr;
    return getMapArea(pHeader);
}

F11_IdentArea_t* Files11Base::GetIdentArea(F11_FileHeader_t* ptr/*=nullptr*/) const
{
    F11_FileHeader_t* pHeader = (ptr == nullptr) ? (F11_FileHeader_t*)m_block : ptr;
    return (F11_IdentArea_t*)((uint16_t*)pHeader + pHeader->fh1_b_idoffset);
}

F11_UserAttrArea_t* Files11Base::GetUserAttr(F11_FileHeader_t* ptr/*=nullptr*/) const
{
    F11_FileHeader_t* pHeader = (ptr == nullptr) ? (F11_FileHeader_t*)m_block : ptr;
    return (F11_UserAttrArea_t * ) &pHeader->fh1_w_ufat;
}

DirectoryRecord_t* Files11Base::GetDirectoryRecord(void* ptr /*=nullptr*/) const
{
    return (ptr == nullptr) ? (DirectoryRecord_t*)m_block : (DirectoryRecord_t*) ptr;
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
    if (date[0] == 0)
        fdate = "--";
    else
    {
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
}

void Files11Base::MakeTime(uint8_t* tim, std::string& ftime)
{
    int d = 0;
    ftime.clear();
    if (tim[d] == 0)
        ftime = "--";
    else {
        ftime += tim[d++]; ftime += tim[d++];
        ftime += ':';
        ftime += tim[d++]; ftime += tim[d++];
        ftime += ':';
        ftime += tim[d++]; ftime += tim[d++];
    }
}


void Files11Base::MakeUIC(uint8_t* uic, std::string& strUIC)
{
	strUIC.clear();
    strUIC += '[';
    strUIC += (char)('0' + ((uic[0] >> 6) & 0x07));
	strUIC += (char)('0' + ((uic[0] >> 3) & 0x07));
	strUIC += (char)('0' + (uic[0] & 0x07));
	strUIC += ',';
	strUIC += (char)('0' + ((uic[1] >> 6) & 0x07));
	strUIC += (char)('0' + ((uic[1] >> 3) & 0x07));
	strUIC += (char)('0' + (uic[1] & 0x07));
	strUIC += ']';
}

uint16_t Files11Base::MakeOwner(const char* str)
{
    std::string owner(str);
    if ((owner.front() == '[') && (owner.back() == ']'))
    {
		owner.erase(0, 1);
		owner.pop_back();
        auto pos = owner.find(',');
		if (pos != std::string::npos)
		{
			std::string hi = owner.substr(0, pos);
			std::string lo = owner.substr(pos + 1);
            uint8_t _hi = (uint8_t)strtol(hi.c_str(), NULL, 8);
			uint8_t _lo = (uint8_t)strtol(lo.c_str(), NULL, 8);
			return (uint16_t)((_hi << 8) | _lo);
		}
    }
    return 0;
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

void Files11Base::GetFileProtectionString(uint16_t pro, std::string& prot)
{
    const char* strProtection = "RWED";
    prot.clear();
    for (int i = 0; i < 4; ++i) {
        prot += (i > 0) ? "," : "[";
        for (int j = 0; j < 4; ++j) {
            if ((pro & 0x01) == 0)
                prot += strProtection[j];
            pro >>= 1;
        }
    }
    prot += "]";
    return;
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
            char ctmp = toupper(((i + j) < len) ? src[i + j] : ' ');
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

bool Files11Base::writeHeader(int lbn, std::fstream& istrm, F11_FileHeader_t* pHeader)
{
    pHeader->fh1_w_checksum = CalcChecksum((uint16_t*)pHeader, (sizeof(F11_FileHeader_t) - sizeof(uint16_t)) / 2);
    return writeBlock(lbn, istrm, (uint8_t*)pHeader) != nullptr;
}

bool Files11Base::validateHeader(F11_FileHeader_t* pHeader)
{
    uint16_t checksum = CalcChecksum((uint16_t*)pHeader, (sizeof(F11_FileHeader_t) / 2) - 1);
    return (pHeader->fh1_w_checksum == checksum);
}

F11_MapArea_t* Files11Base::getMapArea(F11_FileHeader_t* ptr)
{
    return (F11_MapArea_t*)((uint16_t*)ptr + ptr->fh1_b_mpoffset);
}

bool Files11Base::CreateExtensionHeader(int lbn, int extFileNumber, F11_FileHeader_t *pHeader, BlockList_t &blkList, std::fstream& istrm)
{
    F11_FileHeader_t* p = ReadHeader(lbn, istrm);
    int seq_number = p->fh1_w_fid_seq + 1;
    memcpy(p, pHeader, F11_BLOCK_SIZE);
    int segment = 0;
    {
        // update extension info to the originam header
        F11_MapArea_t* pMap = (F11_MapArea_t*)GetMapArea(pHeader);
        pMap->ext_FileNumber = extFileNumber;
        pMap->ext_FileSeqNumber = seq_number;
        segment = pMap->ext_SegNumber + 1;
    }
    p->fh1_w_fid_num = extFileNumber;
    p->fh1_w_fid_seq = seq_number;
    F11_UserAttrArea_t* pUser = (F11_UserAttrArea_t*)&p->fh1_w_ufat;
    pUser->ufcs_ffbyte = F11_BLOCK_SIZE;
    pUser->ufcs_highvbn_hi = 0;
    pUser->ufcs_highvbn_lo = 1;
    pUser->ufcs_eofblck_lo = 0;
    pUser->ufcs_eofblck_lo = 1;
    F11_MapArea_t* pMap = (F11_MapArea_t*)GetMapArea(p);
    pMap->ext_FileNumber = 0;
    pMap->ext_FileSeqNumber = 0;
    pMap->ext_RelVolNo = 0;
    pMap->ext_SegNumber = segment;
    pMap->CTSZ = 1;
    pMap->LBSZ = 3;
    pMap->MAX = 0xcc;
    memset(&pMap->pointers, 0, (unsigned int)pMap->MAX * 2);

    int k = 0;
    F11_Format1_t* Ptrs = (F11_Format1_t*)&pMap->pointers;
    for (auto& blk : blkList)
    {
        int nb  = blk.lbn_end - blk.lbn_start;
        int _lbn = blk.lbn_start;
        do {
            pMap->USE += ((pMap->CTSZ + pMap->LBSZ) / 2);
            Ptrs[k].blk_count = (nb < 255) ? nb : 255;
            Ptrs[k].hi_lbn = _lbn >> 16;
            Ptrs[k].lo_lbn = _lbn & 0xFFFF;
            nb  -= Ptrs[k].blk_count;
            _lbn += (Ptrs[k].blk_count + 1);
            k++;
        } while (nb > 0);
    }
    Files11Base::writeHeader(lbn, istrm, p);
    return true;
}

bool Files11Base::getCurrentDirectory(std::string& dir)
{
    dir.clear();
    char* buffer;
    if ((buffer = _getcwd(NULL, 0)) != NULL)
    {
        dir = buffer;
        free(buffer);
    }
    return true;
}

int Files11Base::GetBlockPointers(F11_MapArea_t* pMap, BlockList_t& blklist)
{
    if (pMap->LBSZ == 3) {
		F11_Format1_t* pFmt1 = (F11_Format1_t*)&pMap->pointers;
        for (int i = 0; i < ((pMap->USE * 2) / sizeof(F11_Format1_t)); ++i)
		{
			int lbn = makeLBN(pFmt1[i].hi_lbn, pFmt1[i].lo_lbn);
			blklist.push_back(BlockPtrs_t(lbn, lbn + pFmt1[i].blk_count));
		}
    }
    else if (pMap->LBSZ == 2) {
        F11_Format2_t* pFmt2 = (F11_Format2_t*)&pMap->pointers;
        for (int i = 0; i < ((pMap->USE * 2) / sizeof(F11_Format2_t)); ++i)
        {
            blklist.push_back(BlockPtrs_t(pFmt2[i].lbn, pFmt2[i].lbn + pFmt2[i].blk_count));
        }
    }
    else if (pMap->LBSZ == 4) {
        F11_Format3_t* pFmt3 = (F11_Format3_t*)&pMap->pointers;
        for (int i = 0; i < ((pMap->USE * 2) / sizeof(F11_Format3_t)); ++i)
        {
            int lbn = makeLBN(pFmt3[i].hi_lbn, pFmt3[i].lo_lbn);
            blklist.push_back(BlockPtrs_t(lbn, lbn + pFmt3[i].blk_count));
        }
    }
	return pMap->ext_FileNumber;
}

int Files11Base::GetBlockCount(const BlockList_t& blklist)
{
	int count = 0;
	for (auto& blk : blklist)
	{
		count += blk.lbn_end - blk.lbn_start + 1;
	}
	return count;
}

int Files11Base::StringToInt(const std::string& str)
{
    int base = 10;
    if (str[0] == '0') {
        bool valid = true;
        for (auto c : str) {
            valid = ((c >= '0') && (c <= '7'));
        }
        if (!valid)
            return -1;
        base = 8;
    }
    else if ((str[0] == 'x') || (str[0] == 'X')) {
        bool valid = true;
        for (auto c : str) {
            valid = ((c >= '0') && (c <= '9')) || ((toupper(c) >= 'A') && (toupper(c) <= 'F'));
        }
        if (!valid)
            return -1;
        base = 16;
    }
    else
    {
        bool valid = true;
        for (auto c : str) {
            valid = ((c >= '0') && (c <= '9'));
        }
        if (!valid)
            return -1;
    }
    return strtol(str.c_str(), NULL, base);
}
