#include "ServerImpl.h"
#include "Peer.h"
#include "../../include/nslServer.h"
#include "NetworkObject.h"
#include "../ObjectClassDefinition.h"

namespace nsl {
	namespace server {

		ServerImpl::ServerImpl(Server* userObject, unsigned int applicationId, unsigned short port)
			: userObject(userObject), connection(applicationId, port)
		{
			currentScopeAccessible = false;
		}

		ServerImpl::~ServerImpl(void)
		{
			close();
		}

		void ServerImpl::open(void)
		{
			connection.open();
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

		double ServerImpl::getTime(void)
		{
			return ((double)clock())/CLOCKS_PER_SEC;
		}

		void ServerImpl::updateNetwork(void)
		{
			if (!connection.isOpened()) {
				throw Exception(NSL_EXCEPTION_DISCONNECTED, "NSL: connection closed.");
			}

			double currentTime = getTime();

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

				sendUpdateToPeer(peer, currentScope, ackIndex);
				
				currentScope.clear();
				it++;
			}

			

			// TODO: lock server objects
		}

		void ServerImpl::sendUpdateToPeer(Peer* peer, std::set<NetworkObject*>& scope, int ackIndex)
		{
			int currentSeqIndex = historyBuffer.getCurrentSeqIndex();
			std::vector<NetworkObject*>* seqScope = peer->getScope(currentSeqIndex);
			seqScope->clear();

			Packet* packet = connection.createPacket(peer->getPeerConnection());
			BitStreamWriter* stream = packet->getStream();

			seqNumber seq = historyBuffer.indexToSeq(currentSeqIndex);
			stream->write<Attribute<seqNumber> >(seq);
			if (ackIndex == NSL_UNDEFINED_BUFFER_INDEX) {
				stream->write<Attribute<seqNumber> >(seq);
			} else {
				stream->write<Attribute<seqNumber> >(historyBuffer.indexToSeq(ackIndex));
			}
			stream->write<double64>(historyBuffer.getTime(currentSeqIndex));
			stream->write<Attribute<seqNumber> >(peer->getCustomMessageSeq());

			// create diff part of update
			if (ackIndex != NSL_UNDEFINED_BUFFER_INDEX) {
				std::vector<NetworkObject*>* ackScope = peer->getScope(ackIndex);
				for(std::vector<NetworkObject*>::iterator it = ackScope->begin(); it != ackScope->end(); it++) {

					NetworkObject* o = *it;

					std::set<NetworkObject*>::iterator scopeObject = scope.find(o);
					if (scopeObject == scope.end()) {
						if (o->getDestroyIndex() == currentSeqIndex) {
							stream->writeByte(NSL_OBJECT_FLAG_DELETE);
						} else {
							stream->writeByte(NSL_OBJECT_FLAG_OUT_OF_SCOPE);
						}
					} else {
						scope.erase(scopeObject);
						stream->writeByte(NSL_OBJECT_FLAG_DIFF);
						seqScope->push_back(o);
					}

					byte* newData;	
					unsigned int byteSize = o->getObjectClass()->getByteSize();
					byte* ackData = o->getDataBySeqIndex(ackIndex);
					byte* currentData = o->getDataBySeqIndex(currentSeqIndex);
					newData = new byte[byteSize];
						
					for (unsigned int i = 0; i < byteSize; i++) {
						newData[i] = ackData[i] ^ currentData[i];
					}

					// TODO: NO_CHANGE flag?

					// TODO: count ticks and sometimes call snapshot:
					//stream->writeByte(NSL_OBJECT_FLAG_SNAPSHOT);
					//newData = o->getDataBySeqIndex(currentSeqIndex);

					writeObjectData(o->getObjectClass(), stream, newData);
				}
			}

			// TODO: create and destroy

			// add new objects to packet
			for(std::set<NetworkObject*>::iterator it = scope.begin(); it != scope.end(); it++) {
				NetworkObject* o = (*it);
				seqScope->push_back(o);
				if (o->getCreationIndex() == currentSeqIndex) {
					stream->writeByte(NSL_OBJECT_FLAG_CREATE);
				} else {
					stream->writeByte(NSL_OBJECT_FLAG_IN_SCOPE);
				}

				stream->write<uint16>(o->getObjectClass()->getId());
				stream->write<uint32>(o->getId());
				writeObjectData(o->getObjectClass(), stream, o->getDataBySeqIndex(currentSeqIndex));
			}
			stream->write<uint8>(NSL_OBJECT_FLAG_END_OF_SECTION);


			// custom messages

			// add unacked reliable custom messages
			int index;
			if (peer->hasAck()) {
				index = (historyBuffer.seqToIndex(peer->getLastAck()) + 1) % NSL_PACKET_BUFFER_SIZE_SERVER;
			} else {
				index = peer->getFirstUpdateIndex();
			}

			while (index != currentSeqIndex) {
				pushBufferedMessagesByIndex(stream, index, peer);
				index = (index + 1) % NSL_PACKET_BUFFER_SIZE_SERVER;
			}
				
			// new messages (current index)
			std::vector<std::pair<BitStreamWriter*, bool> > newCustomMessages = peer->getNewCustomMessages();
			if (!newCustomMessages.empty()) {
				stream->write<Attribute<seqNumber> >(historyBuffer.indexToSeq(currentSeqIndex));
				for (std::vector<std::pair<BitStreamWriter*, bool> >::iterator it = newCustomMessages.begin(); it != newCustomMessages.end(); it++) {

					// append data to stream
					unsigned int size;
					byte* data = it->first->toBytes(size);
					if (size > NSL_MAX_CUSTOM_MESSAGE_SIZE) {
						throw Exception(NSL_EXCEPTION_USAGE_ERROR, "NSL: maximal custom message size exceeded");
					}
					stream->write<Attribute<customMessageSizeNumber> >(size);
					stream->write(size, data);

					// store message if it is realiable
					if (it->second) {
						peer->getBufferedCustomMessages(currentSeqIndex).push_back(std::pair<byte*, unsigned int>(data, size));
					}

					delete it->first;
				}
				newCustomMessages.clear();
				stream->write<Attribute<customMessageSizeNumber> >(0);
			}

			packet->send();
		}

		void ServerImpl::pushBufferedMessagesByIndex(BitStreamWriter* stream, int bufferIndex, Peer* peer)
		{
			std::vector<std::pair<byte*, unsigned int> > messages = peer->getBufferedCustomMessages(bufferIndex);
			if (!messages.empty()) {
				stream->write<Attribute<seqNumber> >(historyBuffer.indexToSeq(bufferIndex));
				for(std::vector<std::pair<byte*, unsigned int> >::iterator it = messages.begin(); it != messages.end(); it++) {
					stream->write<Attribute<customMessageSizeNumber> >(it->second);
					stream->write(it->second, it->first);
				}
				stream->write<Attribute<customMessageSizeNumber> >(0);
			}
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

		void ServerImpl::writeObjectData(ObjectClassDefinition* objectClass, BitStreamWriter* stream, byte* data)
		{
			for (unsigned int i = 0; i < objectClass->getAttributeCount(); i++) {
				int size = objectClass->getAttributeDefinition(i)->size;
				int offset = objectClass->getDataOffset(i);
				stream->write(size, data+offset);
			}
		}

	};
};
