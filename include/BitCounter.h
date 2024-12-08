#pragma once
#include <stdint.h>

#define GET_MAX(a,b) ((a>b) ? a : b)
#define GET_MIN(a,b) ((a<b) ? a : b)

class BitCounter
{
public:
	BitCounter(void);
	void Reset(void);
	void Count(const uint8_t data[], const size_t nbBits);
	void FindSmallestBlock(const uint8_t data[], const size_t nbBits, int minSize);
	int GetNbHi(void) const { return iNbHi; };
	int GetNbLo(void) const { return iNbLo; };
	int GetLargestContiguousHi(void) const { return GET_MAX(iLargestContiguousHi, iContiguousHi); };
	int GetLargestContiguousLo(void) const { return GET_MAX(iLargestContiguousLo, iContiguousLo); };
	int GetSmallestContiguousHi(void) const { return GET_MIN(iSmallestContiguousHi, iContiguousHi); };
	int GetSmallestContiguousLo(void) const { return GET_MIN(iSmallestContiguousLo, iContiguousLo); };
	int GetSmallestBlockHi(void) const { return iSmallBlockHi; };
	int GetSmallestBlockLo(void) const { return iSmallBlockLo; };

protected:
	int  iNbHi;
	int  iNbLo;
	int  iLargestContiguousHi;
	int  iLargestContiguousLo;
	int  iSmallestContiguousHi;
	int  iSmallestContiguousLo;
	int  iSmallBlockHi;
	int  iSmallBlockLo;

private:
	bool bLastState;
	int  iContiguousHi;
	int  iContiguousLo;
	int  blockCounter;
};

