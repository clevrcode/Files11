#include "BitCounter.h"

BitCounter::BitCounter(void)
{
    Reset();
}

void BitCounter::Reset(void)
{
    bLastState    = false;
    blockCounter  = 0;
    iNbHi         = 0;
    iNbLo         = 0;
    iContiguousHi = 0;
    iContiguousLo = 0;
    iSmallBlockHi = -1;
    iSmallBlockLo = -1;
    iLargestContiguousHi  = 0;
    iLargestContiguousLo  = 0;
    iSmallestContiguousHi = INT32_MAX;
    iSmallestContiguousLo = INT32_MAX;
}

void BitCounter::Count(const uint8_t data[], const size_t nbBits)
{
    // calculate the number of bytes
    auto nbBytes = (nbBits + 7) / 8;

    // count bits
    int _nbits = 0;
    for (auto i = 0; i < nbBytes; ++i, ++blockCounter)
    {
        uint8_t b = data[i];
        if ((_nbits + 8) <= nbBits)
        {
            if (b == 0xff)
            {
                iNbHi += 8;
                iContiguousHi += 8;
                _nbits += 8;
                if (!bLastState) {
                    if (iContiguousLo > iLargestContiguousLo)
                        iLargestContiguousLo = iContiguousLo;
                    if (iContiguousLo < iSmallestContiguousLo)
                        iSmallestContiguousLo = iContiguousLo;
                    iContiguousLo = 0;
                    bLastState = true;
                }
                continue;
            }
            else if (b == 0)
            {
                iNbLo += 8;
                iContiguousLo += 8;
                _nbits += 8;
                if (bLastState) {
                    if (iContiguousHi > iLargestContiguousHi)
                        iLargestContiguousHi = iContiguousHi;
                    if (iContiguousHi < iSmallestContiguousHi)
                        iSmallestContiguousLo = iContiguousLo;
                    iContiguousHi = 0;
                    bLastState = false;
                }
                continue;
            }
        }

        // count each bit
        for (auto j = 0; (j < 8) && (_nbits < nbBits); j++, _nbits++)
        {
            if (b & 0x01)
            {
                // block is free
                iNbHi++;
                iContiguousHi++;
                if (!bLastState) {
                    if (iContiguousLo > iLargestContiguousLo)
                        iLargestContiguousLo = iContiguousLo;
                    if (iContiguousLo < iSmallestContiguousLo)
                        iSmallestContiguousLo = iContiguousLo;

                    iContiguousLo = 0;
                    bLastState = true;
                }
            }
            else
            {
                // block is used
                iNbLo++;
                iContiguousLo++;
                if (bLastState) {
                    if (iContiguousHi > iLargestContiguousHi)
                        iLargestContiguousHi = iContiguousHi;
                    if (iContiguousHi < iSmallestContiguousHi)
                        iSmallestContiguousLo = iContiguousLo;
                    iContiguousHi = 0;
                    bLastState = false;
                }
            }
            b >>= 1;
        }
    }
}

void BitCounter::FindSmallestBlock(const uint8_t data[], const size_t nbBits, int minSize)
{
    // calculate the number of bytes
    auto nbBytes = (nbBits + 7) / 8;

    // count bits
    int _nbits = 0;
    for (auto i = 0; i < nbBytes; ++i)
    {
        uint8_t b = data[i];
        if ((_nbits + 8) < nbBits)
        {
            if ((b == 0xff) && bLastState)
            {
                iNbHi += 8;
                iContiguousHi += 8;
                iContiguousLo = 0;
                blockCounter += 8;
                _nbits += 8;
                bLastState = true;
                continue;
            }
            else if ((b == 0) && !bLastState)
            {
                iNbLo += 8;
                iContiguousLo += 8;
                iContiguousHi = 0;
                blockCounter += 8;
                _nbits += 8;
                bLastState = false;
                continue;
            }
        }

        // count each bit
        for (auto j = 0; (j < 8) && (_nbits < nbBits); j++, _nbits++, blockCounter++)
        {
            if (b & 0x01)
            {
                // block is free
                iNbHi++;
                iContiguousHi++;
                if (!bLastState) {
                    if ((iContiguousLo < iSmallestContiguousLo)&&(iContiguousLo >= minSize)) {
                        iSmallestContiguousLo = iContiguousLo;
                        iSmallBlockLo = blockCounter - iContiguousLo;
                    }
                    iContiguousLo = 0;
                    bLastState = true;
                }
            }
            else
            {
                // block is used
                iNbLo++;
                iContiguousLo++;
                if (bLastState) {
                    if ((iContiguousHi < iSmallestContiguousHi)&&(iContiguousHi >= minSize)) {
                        iSmallestContiguousHi = iContiguousHi;
                        iSmallBlockHi = blockCounter - iContiguousHi;
                    }
                    iContiguousHi = 0;
                    bLastState = false;
                }
            }
            b >>= 1;
        }
    }
}

