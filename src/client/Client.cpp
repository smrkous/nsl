/*
	delegate function calls to ClientImpl
*/
#include "../../include/nslClient.h"
#include "../ObjectClassDefinition.h"
#include "ClientImpl.h"

namespace nsl {
	Client::Client(short applicationId, short clientPort)
	{
		i = new client::ClientImpl(this, applicationId, clientPort);
	}

	Client::~Client(void)
	{
		delete i;
	}

	void Client::registerObjectClass(ObjectClass* objectClass)
	{
		i->registerObjectClass(new ObjectClassDefinition(objectClass));
	}

	void Client::open(int port, const char* address)
	{
		i->open(port, address);
	}

	void Client::close()
	{
		i->close();
	}

	bool Client::updateNetwork(void)
	{
		return i->updateNetwork();
	}

	BitStreamWriter* Client::createCustomMessage(void)
	{
		return i->createCustomMessage();
	}

	void Client::onMessageAccept(BitStreamReader* stream)
	{
		// default - do nothing
	}

	void Client::flushNetwork(void)
	{
		i->flushNetwork();
	}
}