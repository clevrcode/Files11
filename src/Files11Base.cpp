#include "Files11Base.h"

Files11Base::Files11Base()
{
}

Files11Base::~Files11Base()
{
}

bool Files11Base::ReadBlock()
{
	return false;
}

uint16_t Files11Base::CalcChecksum(uint16_t *buffer, size_t wordCount)
{
	uint16_t checkSum = 0;
	for (size_t i = 0; i < wordCount; i++)
		checkSum += buffer[i];
	return checkSum;
}