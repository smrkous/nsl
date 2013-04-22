/* 
	nsl.h
	Base library file containing common essential definitions.
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
	//Tthis part of code (endianity recognition) was taken from lz4/lz4.c, the licence is in that file
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
	/*
	#if !defined(NSL_BIG_ENDIAN) && !defined(NSL_LITTLE_ENDIAN)
		#if defined(__BYTE_ORDER__)
			#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
				#define NSL_LITTLE_ENDIAN
			#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
				#define NSL_BIG_ENDIAN
			#endif
		#elif defined(_M_IX86) || defined(i386)
			#define NSL_LITTLE_ENDIAN
		#elif defined(__ppc__) || defined(__powerpc__) || defined (PPC)
			#define NSL_BIG_ENDIAN
		#else
			#error "NSL: Endianity not recognized, must be specified manualy by defining macros NSL_BIG_ENDIAN or NSL_LITTLE_ENDIAN"
		#endif
	#endif
	*/

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

