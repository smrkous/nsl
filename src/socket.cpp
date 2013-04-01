// platform detection
#include "socket.h"
#include <iostream>
#include <fstream>
#include <string>
#include <vector>

namespace nsl {

	Address::Address()
	{
		address = 0;
		port = 0;
	}
	
	Address::Address( unsigned char a, unsigned char b, unsigned char c, unsigned char d, unsigned short port )
	{
		this->address = ( a << 24 ) | ( b << 16 ) | ( c << 8 ) | d;
		this->port = port;
	}
	
	Address::Address( unsigned int address, unsigned short port )
	{
		this->address = address;
		this->port = port;
	}
	
	unsigned int Address::getAddress() const
	{
		return address;
	}
	
	unsigned short Address::getPort() const
	{ 
		return port;
	}
	
	bool Address::operator == ( const Address & other ) const
	{
		return address == other.address && port == other.port;
	}
	
	bool Address::operator != ( const Address & other ) const
	{
		return ! ( *this == other );
	}


	
	Socket::Socket()
	{
		socket = 0;
	}
	
	Socket::~Socket()
	{
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
			throw Exception( "NSL: failed to create socket" );
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
			throw Exception( "NSL: failed to bind socket" );
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
				throw Exception( "NSL: failed to set non-blocking socket" );
			}		
		

		return true;
	}
	
	void Socket::close()
	{
		if ( socket != 0 )
		{
			#ifdef PLATFORM_WINDOWS
			closesocket( socket );
			#elif
			close( socket );
			#endif
		}
	}
	
	bool Socket::isOpen() const
	{
		return socket != 0;
	}

	bool Socket::broadcast(const void * data, int size)
	{
	}

	bool Socket::send( const sockaddr_in destination, const void * data, int size )
	{
		assert( data );
		assert( size > 0 );
		
		if ( socket == 0 )
			return false;

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

		//unsigned int address = ntohl( from.sin_addr.s_addr );
		//unsigned int port = ntohs( from.sin_port );

		//sender = Address( address, port );
		sender = from;

		return received_bytes;
	}
};