#include "Files11Base.h"

Files11Base::Files11Base()
{
    memset(m_block, 0, sizeof(m_block));
}

Files11Base::~Files11Base()
{
}

uint8_t* Files11Base::ReadBlock(int lbn, std::ifstream& istrm)
{
    return readBlock(lbn, istrm, m_block);
}

bool Files11Base::ValidateHomeBlock(ODS1_HomeBlock_t* pHome)
{
    uint16_t checksum1 = CalcChecksum((uint16_t*)pHome, (((char*)&pHome->hm1_w_checksum1 - (char*)&pHome->hm1_w_ibmapsize) / 2));
    uint16_t checksum2 = CalcChecksum((uint16_t*)pHome, (((char*)&pHome->hm1_w_checksum2 - (char*)&pHome->hm1_w_ibmapsize) / 2));
    bool bValid = (pHome->hm1_w_checksum1 == checksum1) && (pHome->hm1_w_checksum2 == checksum2);

    bValid &= ((pHome->hm1_w_ibmapsize != 0) && !((pHome->hm1_w_ibmaplbn_hi == 0) && (pHome->hm1_w_ibmaplbn_lo == 0))) &&
        ((pHome->hm1_w_structlev == HM1_C_LEVEL1) || (pHome->hm1_w_structlev == HM1_C_LEVEL2)) &&
        (pHome->hm1_w_maxfiles != 0) && (pHome->hm1_w_cluster == 1);
    return bValid;
}

ODS1_HomeBlock_t* Files11Base::ReadHomeBlock(std::ifstream& istrm)
{
    ODS1_HomeBlock_t* pHome = (ODS1_HomeBlock_t*) ReadBlock(F11_HOME_LBN, istrm);
    if (pHome)
    {
        //----------------------
        // Validate home block
        if (!ValidateHomeBlock(pHome))
            pHome = NULL;
    }
    return pHome;
}

bool Files11Base::ValidateHeader(ODS1_FileHeader_t* pHeader)
{
    uint16_t checksum = CalcChecksum((uint16_t*)pHeader, 255);
    return (checksum == pHeader->fh1_w_checksum);
}


ODS1_FileHeader_t* Files11Base::ReadFileHeader(int lbn, std::ifstream& istrm)
{
    ODS1_FileHeader_t* pHeader = NULL;
    if (ReadBlock(lbn, istrm))
    {
        if (ValidateHeader((ODS1_FileHeader_t*)m_block))
            pHeader = (ODS1_FileHeader_t*)m_block;
    }
    return pHeader;
}

F11_MapArea_t* Files11Base::GetMapArea(void)
{
    F11_MapArea_t* pMap = NULL;
    ODS1_FileHeader_t* pHeader = (ODS1_FileHeader_t*)m_block;
    if (ValidateHeader(pHeader))
        pMap = (F11_MapArea_t*)((uint16_t*)pHeader + pHeader->fh1_b_mpoffset);
    return pMap;
}

F11_IdentArea_t* Files11Base::GetIdentArea(void)
{
    F11_IdentArea_t* pIdent = NULL;
    ODS1_FileHeader_t* pHeader = (ODS1_FileHeader_t*)m_block;
    if (ValidateHeader(pHeader))
        pIdent = (F11_IdentArea_t*)((uint16_t*)pHeader + pHeader->fh1_b_idoffset);
    return pIdent;
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
        str += radix50Chars[(pR50[i] / 40) / 40];
        str += radix50Chars[(pR50[i] / 40) % 40];
        str += radix50Chars[pR50[i] % 40];
    }
    // strip trailing whitespaces
    if (strip && (str.length() > 0))
    {
        while ((str.length() > 0) && (str.back() == ' ')) {
            str.pop_back();
        }
    }
}

int Files11Base::BuildBlockList(int lbn, BlockList_t& blk_list, std::ifstream& istrm)
{
    int count = 0;
    uint8_t expected_segment = 0;
    int current_lbn = lbn;

    // Clear the plock list
    blk_list.clear();

    do {
        ODS1_FileHeader_t Header;
        if (readBlock(current_lbn, istrm, (uint8_t*)&Header) && ValidateHeader(&Header))
        {
            F11_MapArea_t* pMap = (F11_MapArea_t*)((uint16_t*)&Header + Header.fh1_b_mpoffset);
            if (pMap->ext_SegNumber != expected_segment)
            {
                fprintf(stderr, "Invalid segment: expected %d, read %d\n", expected_segment, pMap->ext_SegNumber);
                break;
            }
            count += GetBlockCount(pMap, &blk_list);

            if (pMap->ext_FileNumber > 0)
            {
                current_lbn += (pMap->ext_FileNumber - 1);
                expected_segment++;
            }
            else
                current_lbn = 0;
        }
        else
        {
            fprintf(stderr, "Failed to reab block lbn %d\n", current_lbn);
            break;
        }
    } while (current_lbn > 0);
    return count;
}

int Files11Base::GetBlockCount(F11_MapArea_t * pMap, BlockList_t * pBlkList/*=NULL*/)
{
        int count = 0;
        int ptr_size = 0;

        if (pMap->LBSZ == 3)
            ptr_size = sizeof(pMap->pointers.fm1);
        else if (pMap->LBSZ == 2)
            ptr_size = sizeof(pMap->pointers.fm2);
        else if (pMap->LBSZ == 4)
            ptr_size = sizeof(pMap->pointers.fm3);
        else
            return 0;

        ptr_size /= 2;
        int nbPtrs = pMap->USE / ptr_size;
        uint16_t* p = (uint16_t*)&pMap->pointers;
        
        for (int i = 0; i < nbPtrs; i++)
        {
            PtrsFormat_t *ptrs = (PtrsFormat_t*)p;
            BlockPtrs_t blocks;
            if (pMap->LBSZ == 3)
            {
                count += ptrs->fm1.blk_count + 1;
                if (pBlkList) {
                    blocks.lbn_start = (ptrs->fm1.hi_lbn << 16) + ptrs->fm1.lo_lbn;
                    blocks.lbn_end = blocks.lbn_start + ptrs->fm1.blk_count;
                    pBlkList->push_back(blocks);
                }
            }
            else if (pMap->LBSZ == 2)
            {
                count += ptrs->fm2.blk_count + 1;
                if (pBlkList) {
                    blocks.lbn_start = ptrs->fm2.lbn;
                    blocks.lbn_end = blocks.lbn_start + ptrs->fm2.blk_count;
                    pBlkList->push_back(blocks);
                }
            }
            else if (pMap->LBSZ == 4) {
                count += ptrs->fm3.blk_count + 1;
                if (pBlkList) {
                    blocks.lbn_start = (ptrs->fm3.hi_lbn << 16) + ptrs->fm3.lo_lbn;
                    blocks.lbn_end = blocks.lbn_start + ptrs->fm3.blk_count;
                    pBlkList->push_back(blocks);
                }
            }
            p += ptr_size;
        }
        return count;
    }


uint8_t *Files11Base::readBlock(int lbn, std::ifstream& istrm, uint8_t* blk)
{
    int ofs = lbn * F11_BLOCK_SIZE;
    istrm.seekg(ofs, istrm.beg);
    istrm.read((char*)blk, F11_BLOCK_SIZE);
    return istrm.good() ? blk : NULL;
}

