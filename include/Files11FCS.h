#pragma once
#include "Files11_defs.h"


class Files11FCS
{
public:
	Files11FCS(void);
	Files11FCS(ODS1_UserAttrArea_t* p);
	Files11FCS(const Files11FCS& fcs);
	void Initialize(ODS1_UserAttrArea_t* p);
	int GetUsedBlockCount(void) const { return m_EOF_Block; };
	int GetHighVBN() const { return m_HighVBN; };
	int GetFirstFreeByte(void) const { return m_FirstFreeByte; };
	bool IsVarLengthRecord(void) const { return (m_RecordType & rt_vlr) != 0; };

private:
	int		m_RecordType;
	int		m_RecordAttributes;
	int		m_RecordSize;
	int		m_HighVBN;
	int		m_EOF_Block;
	int		m_FirstFreeByte;
};

