/* nslBitStream.h - classes and constructs required for serializing and deserializing custom attributes
 * Copyright (C) 2013 Petr Smrcek
 * This file is originally a part of the Net Stalker Library
 * For conditions of distribution and use, see copyright notice in nsl.h
 */

#pragma once

#include <string>
#include "nsl.h"
#include "nslReflection.h"

namespace nsl {
	
	namespace client {

		/// Forward declaration of inner library class
		class Connection;
	};

	namespace server {

		/// Forward declaration of inner library class
		class Connection;

		/// Forward declaration of inner library class
		class ServerImpl;
	};

	/// Transfers raw byte data into defined types and current endianity.
	/// If last byte of the stream is not bit aligned, buffer will not recognize it and will present the last few bits as valid data
	class BitStreamReader
	{
	private:
		friend class client::Connection;

		byte* buffer;
		unsigned int bufferLength;
		byte* currentByte;	// the first byte with at least 1 unread bit
		unsigned char bitOffset; // the first unread bit in a byte (0-7)
		bool bound;

		bool isSpaceFor(unsigned int bitCount);
		void checkSpace(unsigned int bitCount);

	public:

		/// Creates BitStream operating over external buffer
		/// If bount is set to false, reader will just read its data in correct format.
		/// If it is set to true, reader will take care of the buffer entirely (deletion etc.)
		NSL_IMPORT_EXPORT
		BitStreamReader(byte* data, unsigned int dataSize, bool bound = false);

		/// If buffer was not bound to stream, it won't be deleted
		NSL_IMPORT_EXPORT
		~BitStreamReader(void);

		/// If you manualy created subreader, the stream memmory was created in library -> destroy the stream by calling this
		/// Do not call on any reader passed by library! Library manages deletion on its own.
		NSL_IMPORT_EXPORT
		void destroy(void);

		/// Is buffer bound to this reader? (will it be deleted upon stream deletion?)
		NSL_IMPORT_EXPORT
		bool isBound(void);

		/// Creates new reader starting at current position and containing only specified amount of data
		/// Old (this) reader will remain unchanged (current position won't change)
		/// if copyData is set to false, new reader will share buffer with the old one
		NSL_IMPORT_EXPORT
		BitStreamReader* createSubreader(unsigned int byteSize, bool copyData = false);

		/// Read single bit from the stream. If the stream contains no more data, exception is thrown.
		NSL_IMPORT_EXPORT
		bool readBit(void);

		/// Read single byte from the stream. If the stream contains no more data, exception is thrown.
		NSL_IMPORT_EXPORT
		byte readByte(void);

		/// Read defined attribute from the stream. If the stream contains no more data, exception is thrown.
		template<class T>
		typename T::Type read(void) {typename T::Type value; read(T::getByteSize(), (byte *)(&value)); return value;};

		/// Read raw data from the stream. If the stream contains no more data, exception is thrown.
		NSL_IMPORT_EXPORT
		void read(unsigned int byteSize, byte* value);

		/// Get number of whole bytes remaining in this stream (until overflow)
		NSL_IMPORT_EXPORT
		unsigned int getRemainingByteSize(void);

		/// Skip given amount of bits, which is much more performance effective then reading and not using them
		NSL_IMPORT_EXPORT
		void skipBits(unsigned int);

		// Move internal counter to the beginning of the stream, so its data may be read again
		// If new streamSize is 0, old streamSize will be used
		NSL_IMPORT_EXPORT
		void resetStream(unsigned int newStreamSize = 0);
	};
	
	template <class T>
	inline nsl::BitStreamReader & operator>>(nsl::BitStreamReader& r, T& v) {
		v = r.read<nsl::Attribute<T> >();
		return r;
	}
	
	template <>
	inline nsl::BitStreamReader & operator>>(nsl::BitStreamReader& r, std::string & str) {
		char c = r.read<nsl::int8>();
		while(c != 0) {
			str.append(1,c);
			c = r.read<nsl::int8>();
		}
		return r;
	}
	

	#define BIT_STREAM_INIT_SIZE 30
	/// Transfers defined types into raw byte data and unified endianity.
	/// Existing byte array may be passed in constructo. The stream will then write all data into this array.
	class BitStreamWriter
	{
	private:
		friend class client::Connection;
		friend class server::Connection;
		friend class server::ServerImpl;

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

		/// Get number of bytes which have at least one filled bit
		NSL_IMPORT_EXPORT
		unsigned int getByteSize(void);

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
		template<class T>
		void write(const typename T::Type & value) throw (Exception) {write(T::getByteSize(), (byte *)(&value));}

		/// Write raw data into the stream. If the buffer is external and overflows, exception is thrown.
		NSL_IMPORT_EXPORT
		void write(unsigned int byteSize, byte* value);

		/// Get number of whole bytes remaining in this stream (until overflow)
		NSL_IMPORT_EXPORT
		int getRemainingByteSize(void);

		/// Append raw data. No endianity check or transformation will be provided.
		NSL_IMPORT_EXPORT
		void writeRaw(unsigned int byteSize, byte* data);
	};

	template <class T>
	inline nsl::BitStreamWriter & operator<<(nsl::BitStreamWriter &w, const T& v) {
		w.write<nsl::Attribute<T> >(v);
		return w;
	}

	template <>
	inline nsl::BitStreamWriter & operator<<(nsl::BitStreamWriter &w, const std::string & str) {
		for(int i = 0; i < str.length(); ++i) {
			w.write<nsl::int8>(str[i]);
		}
		w.write<nsl::int8>(0);
		return w;
	}
};
