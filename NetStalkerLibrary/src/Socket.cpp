// platform detection
#include "Socket.h"
#include <iostream>
#include <fstream>
#include <string>
#include <vector>

namespace nsl {

	unsigned int socketOpenCounter = 0;

	Socket::Socket(void)
	{
		socket = 0;
		if (socketOpenCounter++ == 0) {
			initializeSockets();
		}
	}

	Socket::~Socket(void)
	{
		if (--socketOpenCounter == 0) {
			shutdownSockets();
		}
		close();
	}

	bool Socket::open( unsigned short port )
	{
		assert( !isOpen() );

		// create socket

		socket = ::socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP );

		if ( socket <= 0 )
		{
			socket = 0;
			return false;
			//throw Exception(NSL_EXCEPTION_LIBRARY_ERROR, "NSL: failed to create socket" );
		}

		// bind to port

		sockaddr_in address;
		address.sin_family = AF_INET;
		address.sin_addr.s_addr = INADDR_ANY;
		address.sin_port = htons( (unsigned short) port );

		if ( bind( socket, (const sockaddr*) &address, sizeof(sockaddr_in) ) < 0 )
		{
			socket = 0;
			close();
			return false;
			//throw Exception(NSL_EXCEPTION_LIBRARY_ERROR, "NSL: failed to bind socket" );
		}

		// set non-blocking io

		#ifdef PLATFORM_WINDOWS
			DWORD nonBlocking = 1;
			if ( ioctlsocket( socket, FIONBIO, &nonBlocking ) != 0 )
			{
		#else
			int nonBlocking = 1;
			if ( fcntl( socket, F_SETFL, O_NONBLOCK, nonBlocking ) == -1 )
			{
		#endif
				close();
				return false;
				//throw Exception(NSL_EXCEPTION_LIBRARY_ERROR, "NSL: failed to set non-blocking socket" );
			}


		return true;
	}

	void Socket::close()
	{
		if ( socket != 0 )
		{
			#ifdef PLATFORM_WINDOWS
			closesocket( socket );
			#else
			::close( socket );
			#endif
		}
	}

	bool Socket::isOpen() const
	{
		return socket != 0;
	}
	/*
	bool Socket::broadcast(const void * data, int size)
	{
	}
	*/
	bool Socket::send( const sockaddr_in destination, const void * data, int size )
	{
		assert( data );
		assert( size > 0 );

		if ( socket == 0 )
			return false;

	#ifdef NSL_LOG_PACKETS
		logBytes((byte*)data, size);
	#endif

		int sent_bytes = sendto( socket, (const char*)data, size, 0, (sockaddr*)&destination, sizeof(sockaddr_in) );

		return sent_bytes == size;
	}

	int Socket::receive( sockaddr_in& sender, void * data, int size )
	{
		assert( data );
		assert( size > 0 );

		if ( socket == 0 )
			return false;

		#ifdef PLATFORM_WINDOWS
		typedef int socklen_t;
		#endif

		sockaddr_in from;
		socklen_t fromLength = sizeof( from );

		int received_bytes = recvfrom( socket, (char*)data, size, 0, (sockaddr*)&from, &fromLength );

		if ( received_bytes <= 0 )
			return 0;

		sender = from;

		return received_bytes;
	}
};
