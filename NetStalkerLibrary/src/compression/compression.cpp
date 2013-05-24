#include "compression.h"
#include "lz4.h"
//#include "rle.h"
#ifdef NSL_COMPRESSION_HIGH
#include "zlib/zlib.h"
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
		//return LZ4_compress_limitedOutput((const char*)src, (char*)dest, srclen, destlen);
#endif
		//return RLE_Compress(src, dest, srclen);
		memcpy(dest, src, srclen); return srclen;

		
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
		//return LZ4_decompress_safe((const char *)src, (char *)dest, srclen, destlen);
#endif
		//return RLE_Uncompress(src, dest, srclen, destlen);
		memcpy(dest, src, srclen); return srclen;
	}
};
