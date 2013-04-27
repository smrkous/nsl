#include "ServerImpl.h"
#include "Peer.h"
#include "../../include/nslServer.h"
#include "NetworkObject.h"
#include "../ObjectClassDefinition.h"

namespace nsl {
	namespace server {

		ServerImpl::ServerImpl(Server* userObject, unsigned int applicationId)
			: userObject(userObject), connection(applicationId)
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
			byte* data = new byte[NSL_MAX_CUSTOM_MESSAGE_SIZE];
			creationMetaData = new BitStreamWriter(data, NSL_MAX_CUSTOM_MESSAGE_SIZE);
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
					ObjectFlags flags;

					std::set<NetworkObject*>::iterator scopeObject = scope.find(o);
					if (scopeObject == scope.end()) {
						flags.action = NSL_OBJECT_FLAG_ACTION_DELETE;
						if (o->getDestroyIndex() == currentSeqIndex) {
							flags.scopeDestroy = NSL_OBJECT_FLAG_SD_DEATH;
						} else {
							flags.scopeDestroy = NSL_OBJECT_FLAG_SD_HIDE;
						}
					} else {
						scope.erase(scopeObject);
						flags.action = NSL_OBJECT_FLAG_ACTION_DIFF;
						seqScope->push_back(o);
					}
					*stream << flags;

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
				ObjectFlags flags;
				flags.action = NSL_OBJECT_FLAG_ACTION_CREATE;
				seqScope->push_back(o);
				if (o->getCreationIndex() == currentSeqIndex) {
					flags.scopeCreate = NSL_OBJECT_FLAG_SC_BIRTH;
				} else {
					flags.scopeCreate = NSL_OBJECT_FLAG_SC_SHOW;
				}

				// add custom message if there is some bound
				byte* creationCustomMessage = NULL;
				unsigned int creationCustomMessageSize;
				if (o->getCreationCustomMessage(creationCustomMessage, creationCustomMessageSize)) {
					flags.creationCustomMessage = NSL_OBJECT_FLAG_CM_PRESENT;
				} else {
					flags.creationCustomMessage = NSL_OBJECT_FLAG_CM_EMPTY;
				}

				*stream << flags;
				stream->write<uint16>(o->getObjectClass()->getId());
				stream->write<uint32>(o->getId());

				if (creationCustomMessage != NULL) {
					stream->write<Attribute<customMessageSizeNumber> >(creationCustomMessageSize);
					stream->writeRaw(creationCustomMessageSize, creationCustomMessage);
				}

				writeObjectData(o->getObjectClass(), stream, o->getDataBySeqIndex(currentSeqIndex));
			}
			ObjectFlags flags;
			flags.action = NSL_OBJECT_FLAG_ACTION_END_OF_SECTION;
			*stream << flags;

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
					stream->writeRaw(size, data);

					// store message if it is reliable
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
					stream->writeRaw(it->second, it->first);
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
