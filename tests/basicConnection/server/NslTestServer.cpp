// NSTest.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "include/nslServer.h"
#include <iostream>
#if defined( __WIN32__ ) || defined( WIN32 ) || defined( _WIN32 )
#include <windows.h>
#else
#include <unistd.h>
#define Sleep(x) usleep((x)*1000)
#endif

class Server : public nsl::Server
{
public:
	Server(unsigned int applicationId, unsigned short port) 
		: nsl::Server(applicationId, port) {
	}

	bool onClientConnect(int peer) {
		std::cout << "New peer connected, id: " << peer << "\n";
		return true;
	}

	void onClientDisconnect(int peer) {
		std::cout << "Peer disconnected, id: " << peer << "\n";
	}

	void onMessageAccept(int peer, nsl::BitStreamReader* reader) {
		std::cout << "Custom message accepted, id: " << peer << ", size: " << reader->getRemainingByteSize() << "\n";
	}
};

#if defined( __WIN32__ ) || defined( WIN32 ) || defined( _WIN32 )
int _tmain(int argc, _TCHAR* argv[])
#else
int main(int argc, char * argv[])
#endif
{
	enum ATTRS {MY_UINT};
	enum OBJECTS {MY_TEST_OBJECT};

	// define object
	nsl::ObjectClass o1(MY_TEST_OBJECT);
	o1.defineAttribute<nsl::uint32>(MY_UINT);

	// create and launch server
	Server server(123, 12345);
	server.registerObjectClass(&o1);
	server.open();

	// initial update
	server.updateNetwork();

	// create object
	unsigned int value = 0;
	nsl::ServerObject* o = server.createObject(MY_TEST_OBJECT);
	o->set<nsl::uint32>(MY_UINT, value);

	server.flushNetwork();
	Sleep(500);

	// iterate and alter object
	while(true) {
		server.updateNetwork();
		value += 10;
		o->set<nsl::uint32>(MY_UINT, value);
		std::cout << value << "\n";
		server.flushNetwork();
		Sleep(500);
	}

	server.close();

	return 0;
}



