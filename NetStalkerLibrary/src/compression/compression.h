#pragma once

#include "../configuration.h"

namespace nsl {
	// compression fail returns 0
	// return value is result size
	unsigned int compress(byte* src, byte* dest, unsigned int srclen, unsigned int destlen);

	// decompression fail returns 0
	// return value is result size
	unsigned int decompress(byte* src, byte* dest, unsigned int srclen, unsigned int destlen);
};
