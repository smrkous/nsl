/*
 * Copyright (C) 2013 Petr Smrcek
 * This file is originally a part of the Net Stalker Library
 * For conditions of distribution and use, see copyright notice in nsl.h
 */

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
