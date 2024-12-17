#pragma once
#include "Files11_defs.h"


class Files11FCS
{
public:
	Files11FCS(void);
	Files11FCS(ODS1_UserAttrArea_t* p);
	Files11FCS(const Files11FCS& fcs);
	void Initialize(ODS1_UserAttrArea_t* p);
	uint8_t GetRecordType(void)       const { return m_RecordType; };
	uint8_t GetRecordAttributes(void) const { return m_RecordAttributes; };
	int     GetRecordSize(void)       const { return m_RecordSize; };
	int     GetUsedBlockCount(void)   const { return m_EOF_Block; };
	int     GetEOFBlock(void)         const { return m_EOF_Block; };
	int     GetHighVBN()              const { return m_HighVBN; };
	int     GetFirstFreeByte(void)    const { return m_FirstFreeByte; };
	bool    IsVarLengthRecord(void)   const { return (m_RecordType & rt_vlr) != 0; };
	bool    IsFixedLengthRecord(void) const { return (m_RecordType & rt_fix) != 0; };

private:
	uint8_t m_RecordType;
	uint8_t	m_RecordAttributes;
	int		m_RecordSize;
	int		m_HighVBN;
	int		m_EOF_Block;
	int		m_FirstFreeByte;
};

