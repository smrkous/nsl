#pragma once

#include "configuration.h"

#ifdef NSL_PLATFORM_WINDOWS

	#include <winsock2.h>
	#pragma comment( lib, "ws2_32.lib" )
	typedef int socklen_t;

#else

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <netdb.h>

#endif

#include <assert.h>

namespace nsl {

	inline bool initializeSockets()
	{
		#ifdef NSL_PLATFORM_WINDOWS
	    WSADATA WsaData;
		return WSAStartup( MAKEWORD(2,2), &WsaData ) != NO_ERROR;
		#else
		return true;
		#endif
	}

	inline void shutdownSockets()
	{
		#ifdef NSL_PLATFORM_WINDOWS
		WSACleanup();
		#endif
	}

	struct Address 
	{
		sockaddr_storage address; 
		socklen_t length;
	};

	class Socket
	{
	private:
		int socketId;
		bool opened;
		char host[NI_MAXHOST], service[NI_MAXSERV];
	public:
		Socket(void);
		~Socket(void);
		// port (service) might be NULL
		void open( const char* service );
		void close(void);
		bool isOpen(void) const;
		bool send( const Address target, const byte * buffer, unsigned int dataInBufferSize );
		bool getStringsFromAddress(const Address address, char* &host, char* &service);
		// if port or address strings are NULL, they are considered ommited
		bool getAddressFromStrings(Address& address, const char* host, const char* service);
		unsigned int receive( Address& receiver, byte * buffer, unsigned int bufferSize );
	};
};
