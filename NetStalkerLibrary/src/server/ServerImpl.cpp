/*
 * Copyright (C) 2013 Petr Smrcek
 * This file is originally a part of the Net Stalker Library
 * For conditions of distribution and use, see copyright notice in nsl.h
 */

#include "ServerImpl.h"
#include "Peer.h"
#include "../../include/nslServer.h"
#include "NetworkObject.h"
#include "../ObjectClassDefinition.h"

namespace nsl {
	namespace server {

		ServerImpl::ServerImpl(Server* userObject, unsigned int applicationId)
			: userObject(userObject), connection(applicationId), protocolParser(&historyBuffer)
		{
			currentScopeAccessible = false;
			lastUpdateTime = 0;
		}

		ServerImpl::~ServerImpl(void)
		{
			close();
		}

		void ServerImpl::open(const char* port)
		{
			connection.open(port);
		}

		void ServerImpl::close(void)
		{
			connection.close();
		}

		void ServerImpl::registerObjectClass(ObjectClassDefinition* objectClass)
		{
			if (connection.isOpened()) {
				throw Exception(NSL_EXCEPTION_USAGE_ERROR, "NSL: connection already opened, types cannot be added now");
			}
			objectManager.registerObjectClass(objectClass);
		}

		NetworkObject* ServerImpl::createObject(unsigned int classId)
		{
			return objectManager.createObject(classId, &historyBuffer);
		}

		NetworkObject* ServerImpl::createObject(unsigned int classId, BitStreamWriter*& creationMetaData)
		{
			NetworkObject* o = objectManager.createObject(classId, &historyBuffer);
			creationMetaData = new BitStreamWriter(NSL_MAX_CUSTOM_MESSAGE_SIZE, true);
			unproccessedCreationCustomMessages.insert(std::pair<unsigned int, BitStreamWriter*>(o->getId(), creationMetaData));
			return o;
		}

		void ServerImpl::updateNetwork(double time)
		{
			if (!connection.isOpened()) {
				throw Exception(NSL_EXCEPTION_DISCONNECTED, "NSL: connection closed.");
			}

			// get current time
			double currentTime = (time == 0 ? getTime() : time);
			if (currentTime < lastUpdateTime) {
				throw Exception(NSL_EXCEPTION_USAGE_ERROR, "NSL: invalid time passed to updateNetwork");
			}

			// proccess all incomming messages
			PeerConnection* peer;
			UpdateCode code;
			BitStreamReader* stream;
			while (EMPTY != (code = connection.update(peer, stream, currentTime))) {
				switch(code) {
					case PEER_CONNECT:
					{
						Peer* newPeer = new Peer(peer);
						connectedPeers.insert(std::pair<unsigned int, Peer*>(peer->connectionId, newPeer));
						if (!userObject->onClientConnect(newPeer->getUserObject())) {
							connectedPeers.erase(peer->connectionId);
							connection.disconnect(peer);
							delete newPeer;
						};
						delete stream;
						break;
					}
					case PEER_DISCONNECT:
					{
						std::map<unsigned int, Peer*>::iterator it = connectedPeers.find(peer->connectionId);
						Peer* oldPeer = it->second;
						userObject->onClientDisconnect(oldPeer->getUserObject());
						connectedPeers.erase(it);
						delete oldPeer;
						delete stream;
						break;
					}
					case PEER_UPDATE:
					{
						std::map<unsigned int, Peer*>::iterator it = connectedPeers.find(peer->connectionId);
						Peer* currentPeer = it->second;
						seqNumber lastAck = stream->read<Attribute<seqNumber> >();

						// if the update is not old
						if (!currentPeer->hasAck() || historyBuffer.isSecondSeqGreater(currentPeer->getLastAck(), lastAck)) {
							currentPeer->setLastAck(lastAck);
							seqNumber newCustomMessageSeq = stream->read<Attribute<seqNumber> >();
							seqNumber peerCustomMessageSeq = currentPeer->getCustomMessageSeq();

							// parse custom messages by seqs
							BitStreamReader* userStream;
							while(stream->getRemainingByteSize() > 0) {

								seqNumber messagesSeq = stream->read<Attribute<seqNumber> >();
								unsigned int msgSize = stream->read<Attribute<customMessageSizeNumber> >();
								if (msgSize == 0) {
									continue;
								}

								bool newMessages = historyBuffer.isSecondSeqGreater(peerCustomMessageSeq, messagesSeq);

								while (msgSize != 0) {
									if (newMessages) {
										userStream = stream->createSubreader(msgSize, false);
										userObject->onMessageAccept(currentPeer->getUserObject(), userStream);
									}
									stream->skipBits(msgSize*8);
									msgSize = stream->read<Attribute<customMessageSizeNumber> >();
								}
							}

							currentPeer->setCustomMessageSeq(newCustomMessageSeq);
						}
						delete stream;
						break;
					}
				}
			}

			// process peer timeouts
			while (NULL != (peer = connection.proccessTimeouts(currentTime))) {
				std::map<unsigned int, Peer*>::iterator it = connectedPeers.find(peer->connectionId);
				Peer* oldPeer = it->second;
				userObject->onClientDisconnect(oldPeer->getUserObject());
				connectedPeers.erase(it);
				delete oldPeer;
			}

			historyBuffer.addSeq(currentTime, &objectManager, &connectedPeers);

			lastUpdateTime = currentTime;
			// TODO: unlock server objects
		}

		void ServerImpl::addToScope(ServerObject* object)
		{
			if (!currentScopeAccessible) {
				throw Exception(NSL_EXCEPTION_USAGE_ERROR, "NSL: trying to add object to scope out of context");
			}

			if (object->networkObject->getDestroyIndex() != NSL_UNDEFINED_BUFFER_INDEX) {
				throw Exception(NSL_EXCEPTION_USAGE_ERROR, "NSL: trying to add already destroyed object to scope");
			} else {
				currentScope.insert(object->networkObject);
			}
		}

		void ServerImpl::flushNetwork(void)
		{
			// process object creation metadata custom messages
			for (std::map<unsigned int, BitStreamWriter*>::iterator it = unproccessedCreationCustomMessages.begin();
				it != unproccessedCreationCustomMessages.end(); it++) {
					NetworkObject* o = objectManager.findObjectById(it->first);
					o->setCreationCustomMessage(it->second->buffer, it->second->getByteSize());
					delete &*(it->second);
			}
			unproccessedCreationCustomMessages.clear();

			// send updates for every connected peer
			std::map<unsigned int, Peer*>::iterator it = connectedPeers.begin();
			while(it != connectedPeers.end()) {

				Peer* peer = it->second;
				int ackIndex;

				if (peer->hasAck()) {
					seqNumber ack = peer->getLastAck();
					if (!historyBuffer.isSeqInBounds(ack)) {
						userObject->onClientDisconnect(peer->getUserObject());
						connectedPeers.erase(it++);
						delete peer;
						continue;
					}

					ackIndex = historyBuffer.seqToIndex(ack);
				} else {
					ackIndex = NSL_UNDEFINED_BUFFER_INDEX;
					if (peer->getFirstUpdateIndex() == NSL_UNDEFINED_BUFFER_INDEX) {
						peer->setFirstUpdateIndex(historyBuffer.getCurrentSeqIndex());
					}
				}

				currentScopeAccessible = true;
				if (!userObject->getScope(peer->getUserObject())) {
					
					// send all objects
					currentScope.clear();
					for (std::map<unsigned int, NetworkObject*>::iterator it2 = objectManager.objectsBegin(); it2 != objectManager.objectsEnd(); it2++) {
						if (it2->second->getDestroyIndex() == NSL_UNDEFINED_BUFFER_INDEX) {
							currentScope.insert(it2->second);
						}
					}

				}
				currentScopeAccessible = false;

				Packet* p = connection.createPacket(peer->getPeerConnection());
				protocolParser.writeUpdateToPeer(p->getStream(), peer, currentScope, ackIndex);
				p->send();

				currentScope.clear();
				it++;
			}

			

			// TODO: lock server objects
		}

		BitStreamWriter* ServerImpl::createCustomMessage(nsl::Peer* peer, bool reliable)
		{
			/*std::map<unsigned int, Peer*>::iterator it = connectedPeers.find(peer);
			if (it == connectedPeers.end()) {
				throw Exception(NSL_EXCEPTION_USAGE_ERROR, "NSL: trying to send message to non-existing peer");
			}*/
			BitStreamWriter* stream = new BitStreamWriter();
			//it->second->getNewCustomMessages().push_back(std::pair<BitStreamWriter*, bool>(stream, reliable));
			peer->peer->getNewCustomMessages().push_back(std::pair<BitStreamWriter*, bool>(stream, reliable));
			return stream;
		}

	};
};
