#pragma once

#include "common.h"

#define BIT_STREAM_INIT_SIZE 30

namespace nsl {

	/// Transfers defined types into raw byte data and unified endianity.
	class BitStreamWriter
	{
	private:
		byte* buffer;
		int bufferLength;
		bool externalBuffer; // is inner buffer external or created by buffer (and should be deleted)
		byte* currentByte;	// the first byte with at least 1 free bit
		unsigned char bitOffset; // the first free bit in a byte (0-7)

		bool isSpaceFor(int bitCount);
		void expand(int bitCount);
		void ensureSpace(int bitCount);
		void write(int bitSize, byte* value);
	public:

		/// Creates BitStream with internal buffer, which is automatically resized if needed
		BitStreamWriter(void);

		/// Creates BitStream with internal buffer, which is automatically resized if needed
		BitStreamWriter(int initialSize);

		/// Creates BitStream operating over external buffer - if the buffer should overflow, exception is thrown
		BitStreamWriter(byte* data, int dataSize);

		/// Buffer is deleted only if it is internal
		~BitStreamWriter(void);

		/// Get copy of internal buffer (only filled part of it)
		/// If there reamin any empty bits in the last byte, it won't be noted
		byte* toBytes(int& byteSize);

		void writeBit(bool);
		void writeByte(byte);
		
		template<class T>
		void write(T::Type value) {write(T::getByteSize(), (byte *)(&value));}
	};

};