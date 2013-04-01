/* 
	nsl.h
	Base library file containing common essential definitions.
*/

#pragma once

#include <exception>

namespace nsl {

	/// Define import or export
	#if defined( __WIN32__ ) || defined( WIN32 ) || defined( _WIN32 )
		#if defined( NSL_BUILD_DLL )
			#define NSL_IMPORT_EXPORT __declspec(dllexport)
		#else
			#define NSL_IMPORT_EXPORT __declspec(dllimport)
		#endif
	#else
		#define NS_IMPORT_EXPORT
	#endif


	/// Endianity recognition
	#if !defined(NSL_BIG_ENDIAN) && !defined(NSL_LITTLE_ENDIAN)
		#if defined(_M_IX86) || defined(i386)
			#define NSL_LITTLE_ENDIAN
		#elif defined(__ppc__) || defined(__powerpc__) || defined (PPC)
			#define NSL_BIG_ENDIAN
		#else
			#error "NSL: Endianity not recognized, must be specified manualy by macros NSL_BIG_ENDIAN or NSL_LITTLE_ENDIAN"
		#endif
	#endif


	/// Basic raw data unit - for custom data definitions
	typedef unsigned char byte;


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
};

