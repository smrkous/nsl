/*
   Net Stalker Library - Library for low latency environment transfer
   Copyright (C) 2013 Petr Smrcek
   BSD 2-Clause License (http://www.opensource.org/licenses/bsd-license.php)

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are
   met:

       * Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
       * Redistributions in binary form must reproduce the above
   copyright notice, this list of conditions and the following disclaimer
   in the documentation and/or other materials provided with the
   distribution.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

   You can contact the author at :
   - NSL source repository : https://github.com/smrkous/nsl
*/

#pragma once

#include <exception>

namespace nsl {

	/* Platform recognition */

	#if defined( __WIN32__ ) || defined( WIN32 ) || defined( _WIN32 )
		#define NSL_PLATFORM_WINDOWS
	#elif defined(__APPLE__)
		#define NSL_PLATFORM_MAC
	#else
		#define NSL_PLATFORM_UNIX
	#endif

	/// Define import or export

	#if defined NSL_PLATFORM_WINDOWS && !defined NSL_REMOVE_DLL_HEADERS
		#if defined( NSL_BUILD_DLL )
			#define NSL_IMPORT_EXPORT __declspec(dllexport)
		#else
			#define NSL_IMPORT_EXPORT __declspec(dllimport)
		#endif
	#else
		#define NSL_IMPORT_EXPORT
	#endif


	/* Endianity recognition */
	// Overwrite the #define below if you know your architecture endianess
	// This part of code (endianity recognition) was taken from lz4.c, the licence is in that file
	#if defined (__GLIBC__)
	#  include <endian.h>
	#  if (__BYTE_ORDER == __BIG_ENDIAN)
	#     define NSL_BIG_ENDIAN
	#  endif
	#elif (defined(__BIG_ENDIAN__) || defined(__BIG_ENDIAN) || defined(_BIG_ENDIAN)) && !(defined(__LITTLE_ENDIAN__) || defined(__LITTLE_ENDIAN) || defined(_LITTLE_ENDIAN))
	#  define NSL_BIG_ENDIAN
	#elif defined(__sparc) || defined(__sparc__) \
	   || defined(__ppc__) || defined(_POWER) || defined(__powerpc__) || defined(_ARCH_PPC) || defined(__PPC__) || defined(__PPC) || defined(PPC) || defined(__powerpc__) || defined(__powerpc) || defined(powerpc) \
	   || defined(__hpux)  || defined(__hppa) \
	   || defined(_MIPSEB) || defined(__s390__)
	#  define NSL_BIG_ENDIAN
	#else
	#define NSL_LITTLE_ENDIAN
	// Little Endian assumed. PDP Endian and other very rare endian format are unsupported.
	#endif

	/// Basic raw data unit - for custom data definitions
	typedef unsigned char byte;


	/// Custom message limitations
	#define NSL_MAX_CUSTOM_MESSAGE_SIZE 256
	typedef unsigned char customMessageSizeNumber;	// must be able to contain NSL_MAX_CUSTOM_MESSAGE_SIZE


	/// Net Stalker Library base exception
	class NSL_IMPORT_EXPORT Exception: public std::exception
	{
	public:
		Exception(int code, const char* m="Net Stalker Library exception"):msg(m),code(code){}
		~Exception(void) throw() {};
		const char* what() const throw() {return msg;}
		int getCode(void) {return code;}
	private:
		const char* msg;
		int code;
	};

	#define NSL_EXCEPTION_DISCONNECTED 1
	#define NSL_EXCEPTION_LIBRARY_ERROR 2
	#define NSL_EXCEPTION_USAGE_ERROR 3


	/// get endianity, for which the librray was compiled
	/// return true for little endian, false for big endian
	NSL_IMPORT_EXPORT
	bool isLittleEndian(void);
};

