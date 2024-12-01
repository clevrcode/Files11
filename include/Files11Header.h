#pragma once

#include <stdint.h>
#include <string>
#include "Files11_defs.h"
#include "Files11Base.h"

class Files11Header : public Files11Base
{
public:
	Files11Header(void);
	Files11Header(const Files11Header &);

	bool Initialize(ODS1_FileHeader_t* pHeader);

private:

};

