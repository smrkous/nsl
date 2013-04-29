/*
	delegate function calls to ClientImpl
*/
#include "../../include/nslClient.h"
#include "../ObjectClassDefinition.h"
#include "ClientImpl.h"

namespace nsl {
	Client::Client(unsigned short applicationId)
	{
		i = new client::ClientImpl(this, applicationId);
	}

	Client::~Client(void)
	{
		delete i;
	}

	void Client::registerObjectClass(ObjectClass& objectClass)
	{
		i->registerObjectClass(new ObjectClassDefinition(objectClass));
	}

	void Client::open(const char* address, const char* port, const char* clientPort)
	{
		i->open(address, port, clientPort);
	}

	void Client::close()
	{
		i->close();
	}

	ClientState Client::updateNetwork(double time)
	{
		return i->updateNetwork(time);
	}

	BitStreamWriter* Client::createCustomMessage(bool reliable)
	{
		return i->createCustomMessage(reliable);
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