#include "common.h"

#ifdef PLATFORM_WINDOWS

	#include <winsock2.h>
	#pragma comment( lib, "wsock32.lib" )

#else

	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <fcntl.h>

#endif

#include <assert.h>

namespace nsl {

	class Address
	{
	private:
		unsigned int address;
		unsigned short port;
	public:
		Address();
		Address( unsigned char a, unsigned char b, unsigned char c, unsigned char d, unsigned short port );
		Address( unsigned int address, unsigned short port );
		unsigned int getAddress() const;
		unsigned short getPort() const;
		bool operator == ( const Address & other ) const;
		bool operator != ( const Address & other ) const;
	};

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
		Socket();
		~Socket();
		bool open( unsigned short port ) throw (Exception);
		void close();
		bool isOpen() const;
		bool send( const sockaddr_in destination, const void * data, int size );
		bool broadcast( const void * data, int size );
		int receive( sockaddr_in& sender, void * data, int size );	
	};
};