#include "../include/nslBitStream.h"

namespace nsl {

	BitStreamReader::BitStreamReader(byte* data, int dataSize)
	{
		this->buffer = data;
		this->bufferLength = dataSize;
		this->bitOffset = 0;
		this->currentByte = data;
	}

	BitStreamReader::~BitStreamReader(void)
	{
	}

	bool BitStreamReader::isSpaceFor(int bitCount) 
	{
		return (bufferLength - (currentByte - buffer))*8 - bitOffset >= bitCount;
	}

	void BitStreamReader::checkSpace(int bitCount)
	{
		if (!isSpaceFor(bitCount)) {
			throw new Exception(NSL_EXCEPTION_USAGE_ERROR, "BitStream contains no more data.");
		}
	}

	bool BitStreamReader::readBit(void)
	{
		checkSpace(1);

		bool result = *currentByte & (1 << bitOffset);

		if (++bitOffset >= 8) {
			currentByte++;
			bitOffset = 0;
		}

		return result;
	}

	byte BitStreamReader::readByte(void)
	{
		checkSpace(8);

		// if there is no bit offset, just copy the byte
		if (bitOffset <= 0) {
			return *(currentByte++);
		} else {
			byte firstPart = *(currentByte++) << bitOffset;
			return firstPart | (*(currentByte) >> (8 - bitOffset));
		}
	}

	void BitStreamReader::read(int byteSize, byte* value)
	{
		if (!byteSize) {
			return;
		}

		checkSpace(byteSize*8);

		// if there is no bit offset, bytes can by directly copied
		if(!bitOffset) 
		{
			#ifdef NSL_BIG_ENDIAN
				byteSize--;
				for(; byteSize >= 0; byteSize--) {
					*(value+byteSize) = *currentByte++;
				}
			#else
				memcpy(value, currentByte, byteSize);
				currentByte += byteSize;
			#endif
			return;
		}

		// there is a bit offset, every byte must be merged
		int bitFreeEnd = 8 - bitOffset;
		byte sourceByte = *currentByte << bitOffset;

		byteSize--;
		#ifndef NSL_BIG_ENDIAN
			byte* targetPointer = value;
		#endif
		for(; byteSize >= 0; byteSize--)
		{
			byte nextByte = *++currentByte;
			#ifdef NSL_BIG_ENDIAN
				*(value+byteSize) = sourceByte | (nextByte >> bitFreeEnd);
			#else
				*targetPointer++ = sourceByte | (nextByte >> bitFreeEnd);
			#endif
			sourceByte = nextByte << bitOffset;
		}
	}

	int BitStreamReader::getRemainingByteSize(void)
	{
		if (bitOffset > 0) {
			return (bufferLength - (currentByte - buffer)) - 1;
		} else {
			return (bufferLength - (currentByte - buffer));
		}
	}

};
