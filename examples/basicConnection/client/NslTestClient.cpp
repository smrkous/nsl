// NSTest.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "nslClient.h"
#include <iostream>
#if defined( __WIN32__ ) || defined( WIN32 ) || defined( _WIN32 )
#include <windows.h>
#else
#include <unistd.h>
#define Sleep(x) usleep((x)*1000)
#endif
#include <map>

std::map<unsigned int, nsl::ClientObject*> objects;

class Client : public nsl::Client
{
public:
	Client(unsigned short applicationId, unsigned short clientPort)
		: nsl::Client(applicationId, clientPort) {
	}

	void onCreate(nsl::ClientObject* object, bool objectBirth) {
		std::cout << "New object created, id: " << object->getId() << ", classId: " << object->getObjectClassId() << "\n";
		objects.insert(std::pair<unsigned int, nsl::ClientObject*>(object->getId(), object));
	}

	void onDestroy(nsl::ClientObject* object, bool objectDeath) {
		std::cout << "Object destroyed, id: " << object->getId() << ", classId: " << object->getObjectClassId() << "\n";
		objects.erase(object->getId());
	}
};

/*
nsl::byte* interpolateMyFloat(nsl::byte** data, double* times, double targetTime) {
	return NULL;
}

nsl::byte* interpolateMyFlag(nsl::byte** data, double* times, double targetTime) {
	return NULL;
}
*/

#if defined( __WIN32__ ) || defined( WIN32 ) || defined( _WIN32 )
int _tmain(int argc, _TCHAR* argv[])
#else
int main(int argc, char * argv[])
#endif
{
	/*try {*/
	enum ATTRS {MY_UINT, MY_UINT2, MY_UINT3, MY_UINT4, MY_UINT5, MY_UINT6, MY_UINT7};
	enum OBJECTS {MY_TEST_OBJECT};

	// define object
	nsl::ObjectClass o1(MY_TEST_OBJECT);
	o1.defineAttribute<nsl::uint32>(MY_UINT);
	o1.defineAttribute<nsl::uint32>(MY_UINT);
	o1.defineAttribute<nsl::uint32>(MY_UINT2);
	o1.defineAttribute<nsl::uint32>(MY_UINT3);
	o1.defineAttribute<nsl::uint32>(MY_UINT4);
	o1.defineAttribute<nsl::uint32>(MY_UINT5);
	o1.defineAttribute<nsl::uint32>(MY_UINT6);
	o1.defineAttribute<nsl::uint32>(MY_UINT7);

  	// create and launch server

	Client client(123, 58765);
	client.registerObjectClass(&o1);
	client.open(12345, "127.0.0.1");

	unsigned int value = 0;

	while(true) {
		nsl::ClientState clientState = client.updateNetwork();
		if (clientState != nsl::ClientState::NSL_CS_OPENED) {
			switch(clientState) {
			case nsl::ClientState::NSL_CS_CLOSED:
				std::cout << "Connecting...\n";
				Sleep(1000);
				break;
			case nsl::ClientState::NSL_CS_HANDSHAKING:
				std::cout << "Handshaking...\n";
				Sleep(400);
				break;			
			case nsl::ClientState::NSL_CS_BUFFERING:
				std::cout << "Buffering data...\n";
				Sleep(400);
				break;
			}
		} else {
			std::cout << ++value << ":";
			for(std::map<unsigned int, nsl::ClientObject*>::iterator it = objects.begin(); it != objects.end(); it++) {
				std::cout << " [" << it->first << "]:" << it->second->get<nsl::uint32>(MY_UINT);
			}
			std::cout << "\n";
			client.flushNetwork();
			Sleep(400);
		}
	}


	client.close();
	/*} catch (nsl::Exception e) {
		std::cout << "Exception has been thrown: ";
		std::cout << e.what();
		std::getchar();
	} catch (std::exception e) {
		std::cout << "Exception has been thrown: ";
		std::cout << e.what();
		std::getchar();
	}*/
	return 0;
}



