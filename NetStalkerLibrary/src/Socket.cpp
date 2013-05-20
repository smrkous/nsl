#include "Socket.h"
#include <iostream>
#include <fstream>
#include <string>
#include <vector>

#ifndef NSL_PLATFORM_WINDOWS
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#else 
#include <Ws2tcpip.h>
#endif

namespace nsl {

	unsigned int socketOpenCounter = 0;

	Socket::Socket(void)
	{
		opened = false;
		if (socketOpenCounter++ == 0) {
			initializeSockets();
		}
#ifdef NSL_PACKET_LOSS && NSL_PACKET_LOSS_SEED
		srand(NSL_PACKET_LOSS_SEED);
#endif
	}

	Socket::~Socket(void)
	{
		if (--socketOpenCounter == 0) {
			shutdownSockets();
		}
		close();
	}

	void Socket::open( const char* port )
	{
		if (isOpen()) {
			throw Exception(NSL_EXCEPTION_LIBRARY_ERROR, "NSL: opening already opened socket" );
		}
		
		// linux part

		struct addrinfo hints;
		struct addrinfo *result, *rp;
		int returnCode;
		struct sockaddr_storage peer_addr;
		

		memset(&hints, 0, sizeof(struct addrinfo));
#ifdef NSL_IPV6
		hints.ai_family = AF_UNSPEC;    // Allow IPv4 or IPv6
#else
		hints.ai_family = AF_INET;    // Allow IPv4 or IPv6
#endif
		hints.ai_socktype = SOCK_DGRAM; // Datagram socket
		hints.ai_flags = AI_PASSIVE;    // For wildcard IP address
		/*hints.ai_protocol = 0;          // Any protocol 
		hints.ai_canonname = NULL;
		hints.ai_addr = NULL;
		hints.ai_next = NULL;*/

		returnCode = getaddrinfo(NULL, port, &hints, &result);
		if (returnCode != 0) {
			throw Exception(NSL_EXCEPTION_USAGE_ERROR, "NSL: wrong ip address provided.");
		}

		for (rp = result; rp != NULL; rp = rp->ai_next) {
			socketId = ::socket(rp->ai_family, rp->ai_socktype,	rp->ai_protocol);
			if (socketId == -1) {
				continue;
			}

			/*int flag = 1;
			returnCode = setsockopt(socketId, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(int));
			if (returnCode == -1) {
				fprintf(stderr, "setsockopt() failed, %s\n", strerror(errno));
				continue;
			}*/

			/*int flag = 0;     
			setsockopt(socketId, IPPROTO_IPV6, IPV6_V6ONLY, (const char *)&flag, sizeof(flag)); 
			*/
			#ifdef NSL_PLATFORM_WINDOWS
				DWORD nonBlocking = 1;
				if ( ioctlsocket( socketId, FIONBIO, &nonBlocking ) != 0 ) {
			#else
				int nonBlocking = 1;
				if ( fcntl( socketId, F_SETFL, O_NONBLOCK, nonBlocking ) == -1 ) {
			#endif
					continue;
				}


			if (bind(socketId, rp->ai_addr, rp->ai_addrlen) == 0) {
				break;
			}

			close();
		}

		if (rp == NULL) {               /* No address succeeded */
			throw Exception(NSL_EXCEPTION_USAGE_ERROR, "NSL: cannot bind address to socket.");
		}

#ifdef NSL_LOG_PACKETS
		char* host, *port2;
		Address address;
		memcpy(&(address.address), rp->ai_addr, rp->ai_addrlen);
		address.length = rp->ai_addrlen;
		getStringsFromAddress(address, host, port2);
		logString(host);
		logString(":");
		logString(port);
		logString("\n");
#endif

		freeaddrinfo(result);
		opened = true;
	}		

	void Socket::close()
	{
		if ( !isOpen() )
		{
			#ifdef NSL_PLATFORM_WINDOWS
			closesocket( socketId );
			#else
			::close( socketId );
			#endif
			opened = false;
		}
	}

	bool Socket::isOpen() const
	{
		return opened;
	}

	bool Socket::send( const Address address, const byte * data, unsigned int size )
	{
		if (!isOpen()) {
			throw Exception(NSL_EXCEPTION_USAGE_ERROR, "NSL: trying to send data trought closed socket");
		}

		if (size == 0) {
			throw Exception(NSL_EXCEPTION_USAGE_ERROR, "NSL: zero size of sent data");
		}

#ifdef NSL_PACKET_LOSS
		if (rand()%100 < (unsigned int)(NSL_PACKET_LOSS*100)) {
			std::cout << "packet lost!" << std::endl;
			return true;
		} else {
			std::cout << "sending..." << std::endl;
		}
#endif
		
#ifdef NSL_LOG_PACKETS
		char* host, *port;
		getStringsFromAddress(address, host, port);
		logString(host);
		logString(":");
		logString(port);
		logString(" ");
		logBytes((byte*)data, size);
#endif

		return size == sendto(socketId, (const char*)data, size, 0, (struct sockaddr *) &(address.address), address.length);
    }

	unsigned int Socket::receive( Address& address, byte * data, unsigned int size )
	{

		int nameInfoResult;
		int read_bytes;

        address.length = sizeof(struct sockaddr_storage);
        read_bytes = recvfrom(socketId, (char *)data, size, 0,
			(struct sockaddr *) &(address.address), &(address.length));
        
		if (read_bytes <= 0) {
			return 0;
		}   

	}

	bool Socket::getStringsFromAddress(const Address address, char* &hostResult, char* &serviceResult)
	{
		int nameInfoResult;
		const Address* targetAddress = &address;

		if (address.address.ss_family==AF_INET6) {
			struct sockaddr_in6* addr6=(struct sockaddr_in6*)&address.address;
			if (IN6_IS_ADDR_V4MAPPED(&addr6->sin6_addr)) {
				struct sockaddr_in addr4;
				memset(&addr4,0,sizeof(addr4));
				addr4.sin_family=AF_INET;
				addr4.sin_port=addr6->sin6_port;
				memcpy(&addr4.sin_addr.s_addr,addr6->sin6_addr.s6_addr+12,sizeof(addr4.sin_addr.s_addr));
				Address a2;
				memcpy(&(a2.address),&addr4,sizeof(addr4));
				a2.length=sizeof(addr4);
				targetAddress = &a2;
			}
		}

		nameInfoResult = getnameinfo(
			(struct sockaddr *) &(targetAddress->address), 
			targetAddress->length, 
			host, 
			NI_MAXHOST,
			service, 
			NI_MAXSERV, 
			NI_NUMERICSERV | NI_NUMERICHOST
		);

		if (nameInfoResult == 0) {
			hostResult = new char[strlen(host)];
			strcpy(hostResult, host);
			serviceResult = new char[strlen(service)];
			strcpy(serviceResult, service);

			return true; 
		} else {
			return false;
		}
	}

	bool Socket::getAddressFromStrings(Address& address, const char* host, const char* service)
	{
		struct addrinfo hints, *res;
		int status;

		memset(&hints, 0, sizeof(hints));
		hints.ai_family = AF_UNSPEC;
		hints.ai_socktype = SOCK_DGRAM;
		/* Setting AI_PASSIVE will give you a wildcard address if host is NULL */
		hints.ai_flags = AI_NUMERICHOST | AI_NUMERICSERV | AI_PASSIVE;

		if ((status = getaddrinfo(host, service, &hints, &res) != 0))
		{
			fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
			return false;
		}

		/* Note, we're taking the first valid address, there may be more than one */
		memcpy(&(address.address), res->ai_addr, res->ai_addrlen);
		address.length = res->ai_addrlen;

		freeaddrinfo(res);
		return true;
	}
};
