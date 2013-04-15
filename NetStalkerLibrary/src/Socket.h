#pragma once

#include "configuration.h"

#ifdef NSL_PLATFORM_WINDOWS

	#include <winsock2.h>
	#pragma comment( lib, "wsock32.lib" )

#else

	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <fcntl.h>

#endif

#include <assert.h>

namespace nsl {

	inline bool initializeSockets()
	{
		#ifdef PLATFORM_WINDOWS
	    WSADATA WsaData;
		return WSAStartup( MAKEWORD(2,2), &WsaData ) != NO_ERROR;
		#else
		return true;
		#endif
	}

	inline void shutdownSockets()
	{
		#ifdef PLATFORM_WINDOWS
		WSACleanup();
		#endif
	}

	class Socket
	{
	private:
		int socket;
	public:
		Socket(void);
		~Socket(void);
		bool open( unsigned short port );
		void close(void);
		bool isOpen(void) const;
		bool send( const sockaddr_in destination, const void * data, int size );
		//bool broadcast( const void * data, int size );
		int receive( sockaddr_in& sender, void * data, int size );
	};
};