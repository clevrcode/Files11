#include "Files11FCS.h"

// Default Constructor
Files11FCS::Files11FCS(void)
{
	m_RecordType = 0;
	m_RecordAttributes = 0;
	m_RecordSize = 0;
	m_HighVBN = 0;
	m_EOF_Block = 0;
	m_FirstFreeByte = 0;
}

// Copy constructor
Files11FCS::Files11FCS(const Files11FCS& fcs) : m_RecordType(fcs.m_RecordType), m_RecordAttributes(fcs.m_RecordAttributes), 
	m_RecordSize(fcs.m_RecordSize), m_HighVBN(fcs.m_HighVBN), m_EOF_Block(fcs.m_EOF_Block), m_FirstFreeByte(fcs.m_FirstFreeByte)
{
}

Files11FCS::Files11FCS(F11_UserAttrArea_t* p)
{
	Initialize(p);
}

void Files11FCS::Initialize(F11_UserAttrArea_t* p)
{
	m_RecordType = p->ufcs_rectype;
	m_RecordAttributes = p->ufcs_recattr;
	m_RecordSize = p->ufcs_recsize;
	m_HighVBN = (p->ufcs_highvbn_hi << 16) + p->ufcs_highvbn_lo;
	m_EOF_Block = (p->ufcs_eofblck_hi << 16) + p->ufcs_eofblck_lo;
	m_FirstFreeByte = p->ufcs_ffbyte;
	if ((m_FirstFreeByte == 0)&&(m_EOF_Block > m_HighVBN)) {
		m_EOF_Block--;
		m_FirstFreeByte = 0x200;
	}
}

