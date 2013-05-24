/*
 * Copyright (C) 2013 Petr Smrcek
 * This file is originally a part of the Net Stalker Library
 * For conditions of distribution and use, see copyright notice in nsl.h
 */

#include <string.h>
#include "../include/nslBitStream.h"

namespace nsl {

	BitStreamReader::BitStreamReader(byte* data, unsigned int dataSize, bool bound)
	{
		this->buffer = data;
		this->bufferLength = dataSize;
		this->bitOffset = 0;
		this->currentByte = data;
		this->bound = bound;
	}

	BitStreamReader::~BitStreamReader(void)
	{
		if (bound) {
			delete[] buffer;
		}
	}

	bool BitStreamReader::isSpaceFor(unsigned int bitCount) 
	{
		return (bufferLength - (currentByte - buffer))*8 - bitOffset >= bitCount;
	}

	void BitStreamReader::checkSpace(unsigned int bitCount)
	{
		if (!isSpaceFor(bitCount)) {
			throw Exception(NSL_EXCEPTION_USAGE_ERROR, "BitStream contains not enough data.");
		}
	}

	bool BitStreamReader::isBound(void)
	{
		return bound;
	}

	BitStreamReader* BitStreamReader::createSubreader(unsigned int byteSize, bool copyData)
	{
		checkSpace(byteSize*8);

		byte* data;
		if (copyData) {
			data = new byte[byteSize];
			memcpy(data, currentByte, byteSize);
		} else {
			data = currentByte;
		}

		BitStreamReader* subreader = new BitStreamReader(data, byteSize, copyData);
		if (bitOffset > 0) {
			subreader->bitOffset = bitOffset;
			subreader->bufferLength++;
		}

		return subreader;
	}

	void BitStreamReader::skipBits(unsigned int bits)
	{
		checkSpace(bits);

		unsigned int bitOffsetAddition = bits % 8;
		bitOffset += bitOffsetAddition;
		if (bitOffset > 7) {
			bitOffset -= 7;
			currentByte++;
		}
		currentByte += (bits - bitOffsetAddition) / 8;
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

	void BitStreamReader::read(unsigned int byteSize, byte* value)
	{
		if (!byteSize) {
			return;
		}

		checkSpace(byteSize*8);

		// if there is no bit offset, bytes can by directly copied
		if(!bitOffset) 
		{
			#ifdef NSL_BIG_ENDIAN
				int index = byteSize - 1;
				for(; index >= 0; index--) {
					*(value+index) = *(currentByte++);
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

	unsigned int BitStreamReader::getRemainingByteSize(void)
	{
		if (bitOffset > 0) {
			return (bufferLength - (currentByte - buffer)) - 1;
		} else {
			return (bufferLength - (currentByte - buffer));
		}
	}

	void BitStreamReader::resetStream(unsigned int newStreamSize)
	{
		currentByte = buffer;
		if (newStreamSize != 0) {
			bufferLength = newStreamSize;
		}
	}

	void BitStreamReader::destroy(void)
	{
		delete this;
	}

};
