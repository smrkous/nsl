/*
 * Copyright (C) 2013 Petr Smrcek
 * This file is originally a part of the Net Stalker Library
 * For conditions of distribution and use, see copyright notice in nsl.h
 */

#include "../../include/nslServer.h"
#include "Peer.h"
#include "Connection.h"

namespace nsl {

	Peer::Peer(server::Peer* peer)
		: peer(peer)
	{}

	Peer::~Peer()
	{}

	unsigned int Peer::getId(void)
	{
		return peer->getPeerConnection()->connectionId;
	}

	const char* Peer::getIp(void)
	{
		// TODO: get peer address
		return "";
	}
};