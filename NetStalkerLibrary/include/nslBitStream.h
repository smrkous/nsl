/* 
	nslBitStream.h
	Contains classes and constructs required for serializing and deserializing custom attributes.
*/

#pragma once

#include "nsl.h"
#include "nslReflexion.h"

namespace nsl {
	
	namespace client {

		/// Forward declaration of inner library class
		class Connection;
	};

	namespace server {

		/// Forward declaration of inner library class
		class Connection;
	};

	/// Transfers raw byte data into defined types and current endianity.
	/// If last byte of the stream is not bit aligned, buffer will not recognize it and will present the last few bits as valid data
	class BitStreamReader
	{
	private:
		byte* buffer;
		unsigned int bufferLength;
		byte* currentByte;	// the first byte with at least 1 unread bit
		unsigned char bitOffset; // the first unread bit in a byte (0-7)

		bool isSpaceFor(unsigned int bitCount);
		void checkSpace(unsigned int bitCount);
	public:

		/// Creates BitStream operating over external buffer
		NSL_IMPORT_EXPORT
		BitStreamReader(byte* data, unsigned int dataSize);

		/// Buffer passed in constructor is never deleted
		NSL_IMPORT_EXPORT
		~BitStreamReader(void);

		/// Creates new reader starting at current position and containing only specified amount of data
		/// Old (this) reader will remain unchanged (current position won't change)
		BitStreamReader* createSubreader(unsigned int byteSize);

		/// Read single bit from the stream. If the stream contains no more data, exception is thrown.
		NSL_IMPORT_EXPORT
		bool readBit(void);

		/// Read single byte from the stream. If the stream contains no more data, exception is thrown.
		NSL_IMPORT_EXPORT
		byte readByte(void);

		/// Read defined attribute from the stream. If the stream contains no more data, exception is thrown.
		template<class T> NSL_IMPORT_EXPORT
		typename T::Type read(void) throw (Exception) {typename T::Type value; read(T::getByteSize(), (byte *)(&value)); return value;};

		/// Read raw data from the stream. If the stream contains no more data, exception is thrown.
		NSL_IMPORT_EXPORT
		void read(unsigned int byteSize, byte* value);

		/// Get number of whole bytes remaining in this stream (until overflow)
		NSL_IMPORT_EXPORT
		unsigned int getRemainingByteSize(void);

		/// Skip given amount of bits, which is much more performance effective then reading and not using them
		NSL_IMPORT_EXPORT
		void skipBits(unsigned int);
	};

	#define BIT_STREAM_INIT_SIZE 30
	/// Transfers defined types into raw byte data and unified endianity.
	/// Existing byte array may be passed in constructo. The stream will then write all data into this array.
	class BitStreamWriter
	{
	private:
		friend class client::Connection;
		friend class server::Connection;

		byte* buffer;
		unsigned int bufferLength;
		bool externalBuffer; // is inner buffer external or created in constructor (and should be deleted)
		byte* currentByte;	// the first byte with at least 1 available bit
		unsigned char bitOffset; // the first free bit in a byte (0-7)

		bool isSpaceFor(unsigned int bitCount);
		void expand(unsigned int bitCount);
		void ensureSpace(unsigned int bitCount);

		// TODO: opimization - add clear()
	public:

		/// Creates BitStream with internal buffer, which is automatically resized if needed
		NSL_IMPORT_EXPORT
		BitStreamWriter(void);

		/// Creates BitStream with internal buffer, which is automatically resized if needed
		NSL_IMPORT_EXPORT
		BitStreamWriter(unsigned int initialSize);

		/// Creates BitStream operating over external buffer - if the buffer should overflow, exception is thrown
		NSL_IMPORT_EXPORT
		BitStreamWriter(byte* data, unsigned int dataSize);

		/// Buffer is deleted only if it is internal
		NSL_IMPORT_EXPORT
		~BitStreamWriter(void);

		/// Append data written into another stream. If the buffer is external and overflows, exception is thrown.
		NSL_IMPORT_EXPORT
		void append(BitStreamWriter* stream);

		/// Get size of filled part in number of bits.
		NSL_IMPORT_EXPORT
		unsigned int getBitSize(void);

		/// Get copy of internal buffer (only filled part of it)
		/// If there reamin any empty bits in the last byte, it won't be noted
		NSL_IMPORT_EXPORT
		byte* toBytes(unsigned int& byteSize);

		/// Write single bit into the stream. If the buffer is external and overflows, exception is thrown.
		NSL_IMPORT_EXPORT
		void writeBit(bool);

		/// Write single byte into the stream. If the buffer is external and overflows, exception is thrown.
		NSL_IMPORT_EXPORT
		void writeByte(byte);
		
		/// Write defined attribute into the stream. If the buffer is external and overflows, exception is thrown.
		template<class T> NSL_IMPORT_EXPORT
		void write(typename T::Type value) throw (Exception) {write(T::getByteSize(), (byte *)(&value));}

		/// Write raw data into the stream. If the buffer is external and overflows, exception is thrown.
		NSL_IMPORT_EXPORT
		void write(unsigned int byteSize, byte* value);

		/// Get number of whole bytes remaining in this stream (until overflow)
		NSL_IMPORT_EXPORT
		int getRemainingByteSize(void);
	};
};
