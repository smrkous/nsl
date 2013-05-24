/*
 * Copyright (C) 2013 Petr Smrcek
 * This file is originally a part of the Net Stalker Library
 * For conditions of distribution and use, see copyright notice in nsl.h
 */

#include "Peer.h"
#include "Connection.h"
#include "../../include/nslBitStream.h"

namespace nsl {
	namespace server {
		Peer::Peer(PeerConnection* peer) : peerConnection(peer)
		{
			customMessageSeq = 0;
			userObject = new nsl::Peer(this);
			isAck = false;
			firstUpdateIndex = NSL_UNDEFINED_BUFFER_INDEX;
		}

		Peer::~Peer(void)
		{
			// TODO	where to delete this struct?
			delete peerConnection;
			delete userObject;
		}

		nsl::Peer* Peer::getUserObject(void)
		{
			return userObject;
		}

		PeerConnection* Peer::getPeerConnection(void)
		{
			return peerConnection;
		}

		seqNumber Peer::getLastAck(void)
		{
			return lastAck;
		}

		void Peer::setLastAck(seqNumber ack)
		{
			lastAck = ack;
			isAck = true;
		}

		bool Peer::hasAck(void)
		{
			return isAck;
		}

		std::vector<NetworkObject*>* Peer::getScope(int bufferIndex)
		{
			return &scope[bufferIndex];
		}

		void Peer::clearIndex(int bufferIndex)
		{
			if (firstUpdateIndex == bufferIndex) {
				firstUpdateIndex = NSL_UNDEFINED_BUFFER_INDEX;
			}

			for (std::vector<std::pair<byte*, unsigned int> >::iterator it = customMessageBuffer[bufferIndex].begin(); it != customMessageBuffer[bufferIndex].end(); it++) {
				delete[] it->first;
			}

			customMessageBuffer[bufferIndex].clear();
		}
	};
};