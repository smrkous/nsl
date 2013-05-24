/*
 * Copyright (C) 2013 Petr Smrcek
 * This file is originally a part of the Net Stalker Library
 * For conditions of distribution and use, see copyright notice in nsl.h
 */

#include "../../include/nslServer.h"
#include "ServerImpl.h"
#include "NetworkObject.h"
#include "../ObjectClassDefinition.h"

namespace nsl {

	Server::Server(unsigned int applicationId)
	{
		i = new server::ServerImpl(this, applicationId);
	}

	Server::~Server(void)
	{
		delete i;
	}

	void Server::open(const char* port)
	{
		i->open(port);
	}

	void Server::close(void)
	{
		i->close();
	}

	void Server::registerObjectClass(ObjectClass& objectClass)
	{
		i->registerObjectClass(new ObjectClassDefinition(objectClass));
	}

	ServerObject* Server::createObject(unsigned short classId)
	{
		return i->createObject(classId)->getUserObject();
	}

	ServerObject* Server::createObject(unsigned short classId, BitStreamWriter*& creationMetaData)
	{
		return i->createObject(classId, creationMetaData)->getUserObject();
	}

	BitStreamWriter* Server::createCustomMessage(nsl::Peer* peer, bool reliable)
	{
		return i->createCustomMessage(peer, reliable);
	}

	void Server::flushNetwork(void)
	{
		i->flushNetwork();
	}

	void Server::updateNetwork(double time)
	{
		i->updateNetwork(time);
	}


	/* overridable methods */

	bool Server::onClientConnect(nsl::Peer* peer)
	{
		//addObjectReceiver(peer);
		return true;
	}

	void Server::onClientDisconnect(nsl::Peer* peer)
	{
		//removeObjectReceiver(peer);
	}

	void Server::onMessageAccept(nsl::Peer* peer, BitStreamReader* stream)
	{
	
	}

	bool Server::getScope(nsl::Peer* peer)
	{
		return false;
	}

	void Server::addToScope(ServerObject* object)
	{
		i->addToScope(object);
	}

};
