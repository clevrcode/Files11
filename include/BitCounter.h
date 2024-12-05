#pragma once
#include <stdint.h>

#define GET_MAX(a,b) ((a>b) ? a : b)

class BitCounter
{
public:
	BitCounter(void);
	void Reset(void);
	void Count(const uint8_t data[], const size_t nbBits);
	int GetNbHi(void) const { return iNbHi; };
	int GetNbLo(void) const { return iNbLo; };
	int GetLargestContiguousHi(void) const { return GET_MAX(iLargestContiguousHi, iContiguousHi); };
	int GetLargestContiguousLo(void) const { return GET_MAX(iLargestContiguousLo, iContiguousLo); };

protected:
	int  iNbHi;
	int  iNbLo;
	int  iLargestContiguousHi;
	int  iLargestContiguousLo;

private:
	bool bLastState;
	int  iContiguousHi;
	int  iContiguousLo;
};

