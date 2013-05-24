/*
 * Copyright (C) 2013 Petr Smrcek
 * This file is originally a part of the Net Stalker Library
 * For conditions of distribution and use, see copyright notice in nsl.h
 */

#include "compression.h"
#ifdef NSL_COMPRESSION_HIGH
#include "zlib/zlib.h"
#else
#include "lz4.h"
#endif

namespace nsl {
	unsigned int compress(byte* src, byte* dest, unsigned int srclen,  unsigned int destlen)
	{
#ifdef NSL_COMPRESSION_HIGH
		uLongf resultSize = destlen;
		unsigned int state = ::compress2(dest, &resultSize, src, srclen,1);
		if (state == Z_OK) {
			return resultSize;
		} else {
			return 0;
		}
#else
		return LZ4_compress_limitedOutput((const char*)src, (char*)dest, srclen, destlen);
#endif
	}

	unsigned int decompress(byte* src, byte* dest, unsigned int srclen, unsigned int destlen)
	{
#ifdef NSL_COMPRESSION_HIGH
		uLongf resultSize = destlen;
		unsigned int state = ::uncompress(dest, &resultSize, src, srclen);
		if (state == Z_OK) {
			return resultSize;
		} else {
			return 0;
		}
#else
		return LZ4_decompress_safe((const char *)src, (char *)dest, srclen, destlen);
#endif
	}
};
