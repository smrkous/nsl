// NSTest.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "nslServer.h"
#include <iostream>
#include <cstdio>
#if defined( __WIN32__ ) || defined( WIN32 ) || defined( _WIN32 )
#include <windows.h>
#else
#include <unistd.h>
#define Sleep(x) usleep((x)*1000)
#endif

class Server : public nsl::Server
{
public:
	Server(unsigned int applicationId) 
		: nsl::Server(applicationId) {
	}

	bool onClientConnect(nsl::Peer* peer) {
		std::cout << "New peer connected, id: " << peer->getId() << "\n";
		return true;
	}

	void onClientDisconnect(nsl::Peer* peer) {
		std::cout << "Peer disconnected, id: " << peer->getId() << "\n";
	}

	void onMessageAccept(nsl::Peer* peer, nsl::BitStreamReader* reader) {
		std::cout << "Custom message accepted, id: " << peer->getId() << ", size: " << reader->getRemainingByteSize() << "\n";
	}
};

#if defined( __WIN32__ ) || defined( WIN32 ) || defined( _WIN32 )
int _tmain(int argc, _TCHAR* argv[])
#else
int main(int argc, char * argv[])
#endif
{
	//try {
	enum ATTRS {MY_UINT, MY_UINT2, MY_UINT3, MY_UINT4, MY_UINT5, MY_UINT6, MY_UINT7};
	enum OBJECTS {MY_TEST_OBJECT};

	// define object
	nsl::ObjectClass o1(MY_TEST_OBJECT);
	o1.defineAttribute<nsl::uint32>(MY_UINT);
	o1.defineAttribute<nsl::uint32>(MY_UINT2);
	o1.defineAttribute<nsl::uint32>(MY_UINT3);
	o1.defineAttribute<nsl::uint32>(MY_UINT4);
	o1.defineAttribute<nsl::uint32>(MY_UINT5);
	o1.defineAttribute<nsl::uint32>(MY_UINT6);
	o1.defineAttribute<nsl::uint32>(MY_UINT7);

	// create and launch server
	Server server(123);
	server.registerObjectClass(&o1);
	server.open("12345");

	// initial update
	server.updateNetwork();

	// create object
	unsigned int value = 0;
	nsl::BitStreamWriter* creationMessage;
	nsl::ServerObject* o = server.createObject(MY_TEST_OBJECT, creationMessage);
	o->set<nsl::uint32>(MY_UINT, value);
	o->set<nsl::uint32>(MY_UINT2, value);
	o->set<nsl::uint32>(MY_UINT3, value);
	o->set<nsl::uint32>(MY_UINT4, value);
	o->set<nsl::uint32>(MY_UINT5, value);
	o->set<nsl::uint32>(MY_UINT6, value);
	o->set<nsl::uint32>(MY_UINT7, value);
	*creationMessage << "novej objekt!";

	nsl::ServerObject* o2 = server.createObject(MY_TEST_OBJECT);
	o->set<nsl::uint32>(MY_UINT, value);
	o->set<nsl::uint32>(MY_UINT2, value);
	o->set<nsl::uint32>(MY_UINT3, value);
	o->set<nsl::uint32>(MY_UINT4, value);
	o->set<nsl::uint32>(MY_UINT5, value);
	o->set<nsl::uint32>(MY_UINT6, value);
	o->set<nsl::uint32>(MY_UINT7, value);

	server.flushNetwork();
	Sleep(500);

	bool secondObjectDestroyed = false;
	// iterate and alter object
	while(true) {
		server.updateNetwork();
		value += 10;
		std::cout << value << "\n";
		o->set<nsl::uint32>(MY_UINT, value);
		// update objects with their last values before destruction, after that the object cannot be managed anymore
		if (!secondObjectDestroyed) {
			o2->set<nsl::uint32>(MY_UINT, value*2);
		}

		if (!secondObjectDestroyed && value > 250) {
			o2->destroy();
			secondObjectDestroyed = true;
			std::cout << "Second object destroyed.\n";
		}
		
		server.flushNetwork();
		Sleep(500);
	}

	server.close();
	//} catch (nsl::Exception e) {
	//	std::cout << "Exception has been thrown: ";
	//	std::cout << e.what() << std::endl;
	//	std::getchar();
	//} catch (std::exception e) {
	//	std::cout << "Exception has been thrown: ";
	//	std::cout << e.what() << std::endl;
	//	std::getchar();
	//}

	return 0;
}



