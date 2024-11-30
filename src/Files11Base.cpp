#include "Files11Base.h"

Files11Base::Files11Base()
{
}

Files11Base::~Files11Base()
{
}

bool Files11Base::ReadBlock(int lbn, std::ifstream& istrm, uint8_t* buf)
{
	istrm.seekg(lbn, istrm.beg);
	istrm.read((char*)buf, F11_BLOCK_SIZE);
	return istrm.good();
}

uint16_t Files11Base::CalcChecksum(uint16_t *buffer, size_t wordCount)
{
	uint16_t checkSum = 0;
	for (size_t i = 0; i < wordCount; i++)
		checkSum += buffer[i];
	return checkSum;
}

void Files11Base::MakeString(char* str, size_t len, std::string &outstr)
{
	char* strBuffer = new char[len + 1];
	memset(strBuffer, 0, len + 1);
	memcpy(strBuffer, str, len);
	outstr = strBuffer;
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
        fdate += ':';
        fdate += date[d++]; fdate += date[d++];
    }
}

void Files11Base::MakeUIC(uint8_t* uic, std::string& strUIC)
{
    
}
