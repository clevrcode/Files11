#pragma once

#include <stdint.h>
#include "Files11_defs.h"


class Files11Base
{
public:
	Files11Base();
	~Files11Base();

protected:
	bool     ReadBlock();
	uint16_t CalcChecksum(uint16_t* buffer, size_t wordCount);


};

