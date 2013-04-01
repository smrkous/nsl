#include "../include/nslBitStream.h"

namespace nsl {

	BitStreamWriter::BitStreamWriter(void)
	{
		this->buffer = new byte[BIT_STREAM_INIT_SIZE];
		this->bufferLength = BIT_STREAM_INIT_SIZE;
		this->bitOffset = 0;
		this->externalBuffer = false;
		this->currentByte = this->buffer;
	}

	BitStreamWriter::BitStreamWriter(int initialSize)
	{
		this->buffer = new byte[initialSize];
		this->bufferLength = initialSize;
		this->bitOffset = 0;
		this->externalBuffer = false;
		this->currentByte = this->buffer;
	}

	BitStreamWriter::BitStreamWriter(byte* data, int dataSize)
	{
		this->buffer = data;
		this->bufferLength = dataSize;
		this->bitOffset = 0;
		this->externalBuffer = true;
		this->currentByte = data;
	}

	BitStreamWriter::~BitStreamWriter(void)
	{
		if (!this->externalBuffer) {
			delete[] buffer;
		}
	}

	void BitStreamWriter::append(BitStreamWriter* stream)
	{
		int byteSize = stream->currentByte - stream->buffer;
		ensureSpace(byteSize*8 + stream->bitOffset);

		write(byteSize, stream->buffer);
		if (stream->bitOffset > 0) {
			write(1, stream->currentByte);
			int bitsToRemove = 8 - stream->bitOffset;
			if (bitsToRemove > bitOffset) {
				currentByte--;
				bitOffset += stream->bitOffset;
			} else {
				bitOffset -= bitsToRemove;
			}
		}
	}

	byte* BitStreamWriter::toBytes(int& byteSize)
	{
		byteSize = (currentByte - buffer);
		if (bitOffset > 0) {
			byteSize++;
		}
		byte* result = new byte[byteSize];
		memcpy(result, buffer, byteSize);
		return result;
	}

	bool BitStreamWriter::isSpaceFor(int bitCount) 
	{
		return (bufferLength - (currentByte - buffer))*8 - bitOffset >= bitCount;
	}

	void BitStreamWriter::expand(int bitCount) 
	{
		byte* tmp = new byte[bufferLength*2 + bitCount/8 + 1];
		memcpy(tmp, buffer, bufferLength);
		currentByte = (currentByte - buffer) + tmp;
		delete[] buffer;
		buffer = tmp;
	}

	void BitStreamWriter::ensureSpace(int bitCount)
	{
		if (!isSpaceFor(bitCount)) {
			if (this->externalBuffer) {
				throw new Exception(NSL_EXCEPTION_USAGE_ERROR, "External buffer overflowed");
			} else {
				expand(bitCount);
			}
		}
	}

	void BitStreamWriter::writeBit(bool bit)
	{
		ensureSpace(1);

		*currentByte = *currentByte & (0xFF << (8 - bitOffset));
		if (bit) {
			*currentByte = *currentByte | (1 << (7 - bitOffset));
		}

		if (++bitOffset >= 8) {
			currentByte++;
			bitOffset = 0;
		}
	}

	void BitStreamWriter::writeByte(byte value)
	{
		ensureSpace(8);

		// if there is no bit offset, just copy the byte
		if (bitOffset <= 0) {
			*(currentByte++) = value;
		} else {
			int bitFreeEnd = 8 - bitOffset;
			*currentByte = (*currentByte & (0xFF << bitFreeEnd)) | (value >> bitOffset);	
			*(++currentByte) = value << bitFreeEnd;
		}
	}

	void BitStreamWriter::write(int byteSize, byte* value)
	{
		if(!byteSize) {
			return;
		}
		
		ensureSpace(byteSize*8);

		// if there is no bit offset, bytes can by directly copied
		if(!bitOffset) 
		{
			#ifdef NSL_BIG_ENDIAN
				byteSize--;
				for(; byteSize >= 0; byteSize--) {
					*(currentByte++) = *(value+byteSize);
				}
			#else
				memcpy(currentByte, value, byteSize);
				currentByte += byteSize;
			#endif
			return;
		}

		// there is a bit offset, every byte must be merged
		int bitFreeEnd = 8 - bitOffset;
		byte sourceByte;
		byte destByte = *currentByte & (0xFF << bitFreeEnd);	// set zeros (from undefined) to bitFreeEnd bits

		byteSize--;
		for(; byteSize >= 0; byteSize--) {
			#ifdef NSL_BIG_ENDIAN
				sourceByte = *(value+byteSize);
			#else
				sourceByte = *(value++);
			#endif
			*(currentByte++) = destByte | sourceByte >> bitOffset;
			destByte = sourceByte << bitFreeEnd;
		}

		*currentByte = destByte;
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
