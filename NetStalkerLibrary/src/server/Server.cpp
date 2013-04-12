#include "../../include/nslServer.h"
#include "ServerImpl.h"
#include "../ObjectClassDefinition.h"

namespace nsl {

	Server::Server(unsigned int applicationId, unsigned short port)
	{
		i = new server::ServerImpl(this, applicationId, port);
	}

	Server::~Server(void)
	{
		delete i;
	}

	void Server::open(void)
	{
		i->open();
	}

	void Server::close(void)
	{
		i->close();
	}

	void Server::registerObjectClass(ObjectClass* objectClass)
	{
		i->registerObjectClass(new ObjectClassDefinition(objectClass));
	}

	BitStreamWriter* Server::createCustomMessage(int peer, bool reliable)
	{
		return i->createCustomMessage(peer, reliable);
	}

	void Server::flushNetwork(void)
	{
		i->flushNetwork();
	}

	void Server::updateNetwork(void)
	{
		i->updateNetwork();
	}


	/* overridable methods */

	bool Server::onClientConnect(int peer)
	{
		//addObjectReceiver(peer);
		return true;
	}

	void Server::onClientDisconnect(int peer)
	{
		//removeObjectReceiver(peer);
	}

	void Server::onMessageAccept(int peer, BitStreamReader* stream)
	{
	
	}

	bool Server::getScope(int peer)
	{
		return false;
	}

	void Server::addToScope(ServerObject* object)
	{
		i->addToScope(object);
	}


};
