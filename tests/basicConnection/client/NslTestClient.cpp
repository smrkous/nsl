// NSTest.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "include/nslClient.h"
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
	enum ATTRS {MY_UINT};
	enum OBJECTS {MY_TEST_OBJECT};

	// define object
	nsl::ObjectClass o1(MY_TEST_OBJECT);
	o1.defineAttribute<nsl::uint32>(MY_UINT);

  	// create and launch server

	Client client(123, 58765);
	client.registerObjectClass(&o1);
	client.open(12345, "127.0.0.1");

	unsigned int value = 0;

	while(true) {
		if (!client.updateNetwork()) {
			std::cout << "Connecting...\n";
			Sleep(1000);
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

	return 0;
}



