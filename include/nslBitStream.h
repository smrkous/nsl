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
	class NSL_IMPORT_EXPORT BitStreamReader
	{
	private:
		byte* buffer;
		int bufferLength;
		byte* currentByte;	// the first byte with at least 1 available bit
		unsigned char bitOffset; // the first free bit in a byte (0-7)

		bool isSpaceFor(int bitCount);
		void checkSpace(int bitCount);
	public:

		/// Creates BitStream operating over external buffer
		BitStreamReader(byte* data, int dataSize);

		/// Buffer passed in constructor is never deleted
		~BitStreamReader(void);

		/// Read single bit from the stream. If the stream contains no more data, exception is thrown.
		bool readBit(void) throw (Exception);

		/// Read single byte from the stream. If the stream contains no more data, exception is thrown.
		byte readByte(void) throw (Exception);

		/// Read defined attribute from the stream. If the stream contains no more data, exception is thrown.
		template<class T>
		T::Type read(void) throw (Exception) {T::Type value; read(T::getByteSize(), (byte *)(&value)); return value;}

		/// Read raw data from the stream. If the stream contains no more data, exception is thrown.
		void read(int byteSize, byte* value);

		/// Get number of whole bytes remaining in this stream (until overflow)
		int getRemainingByteSize(void);
	};

	#define BIT_STREAM_INIT_SIZE 30
	/// Transfers defined types into raw byte data and unified endianity.
	/// Existing byte array may be passed in constructo. The stream will then write all data into this array.
	class NSL_IMPORT_EXPORT BitStreamWriter
	{
	private:
		friend class client::Connection;

		byte* buffer;
		int bufferLength;
		bool externalBuffer; // is inner buffer external or created in constructor (and should be deleted)
		byte* currentByte;	// the first byte with at least 1 unread bit
		unsigned char bitOffset; // the first unread bit in a byte (0-7)

		bool isSpaceFor(int bitCount);
		void expand(int bitCount);
		void ensureSpace(int bitCount);
	public:

		/// Creates BitStream with internal buffer, which is automatically resized if needed
		BitStreamWriter(void);

		/// Creates BitStream with internal buffer, which is automatically resized if needed
		BitStreamWriter(int initialSize);

		/// Creates BitStream operating over external buffer - if the buffer should overflow, exception is thrown
		BitStreamWriter(byte* data, int dataSize);

		/// Buffer is deleted only if it is internal
		~BitStreamWriter(void);

		/// Append data written into another stream. If the buffer is external and overflows, exception is thrown.
		void append(BitStreamWriter* stream);

		/// Get copy of internal buffer (only filled part of it)
		/// If there reamin any empty bits in the last byte, it won't be noted
		byte* toBytes(int& byteSize);

		/// Write single bit into the stream. If the buffer is external and overflows, exception is thrown.
		void writeBit(bool) throw (Exception);

		/// Write single byte into the stream. If the buffer is external and overflows, exception is thrown.
		void writeByte(byte) throw (Exception);
		
		/// Write defined attribute into the stream. If the buffer is external and overflows, exception is thrown.
		template<class T>
		void write(T::Type value) throw (Exception) {write(T::getByteSize(), (byte *)(&value));}

		/// Write raw data into the stream. If the buffer is external and overflows, exception is thrown.
		void write(int byteSize, byte* value);

		/// Get number of whole bytes remaining in this stream (until overflow)
		int getRemainingByteSize(void);
	};
};
