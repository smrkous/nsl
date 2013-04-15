/* 
	nsl.h
	Base library file containing common essential definitions.
*/

#pragma once

#include <exception>

namespace nsl {
#define NSL_BUILD_DLL
	/// Define import or export
	#if defined( __WIN32__ ) || defined( WIN32 ) || defined( _WIN32 )
		#if defined( NSL_BUILD_DLL )
			#define NSL_IMPORT_EXPORT __declspec(dllexport)
		#else
			#define NSL_IMPORT_EXPORT __declspec(dllimport)
		#endif
	#else
		#define NSL_IMPORT_EXPORT
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
};

