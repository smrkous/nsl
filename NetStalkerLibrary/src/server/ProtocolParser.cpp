#include "ProtocolParser.h"
#include "HistoryBuffer.h"
#include "Peer.h"
#include "../ObjectClassDefinition.h"
#include "NetworkObject.h"

namespace nsl {
	namespace server {
		ProtocolParser::ProtocolParser(HistoryBuffer* historyBuffer) : historyBuffer(historyBuffer)
		{}

		ProtocolParser::~ProtocolParser(void)
		{}

		void ProtocolParser::writeUpdateToPeer(BitStreamWriter* stream, Peer* peer, std::set<NetworkObject*>& scope, int ackIndex)
		{
			int currentSeqIndex = historyBuffer->getCurrentSeqIndex();
			std::vector<NetworkObject*>* seqScope = peer->getScope(currentSeqIndex);
			seqScope->clear();

			seqNumber seq = historyBuffer->indexToSeq(currentSeqIndex);
			stream->write<Attribute<seqNumber> >(seq);
			if (ackIndex == NSL_UNDEFINED_BUFFER_INDEX) {
				stream->write<Attribute<seqNumber> >(seq);
			} else {
				stream->write<Attribute<seqNumber> >(historyBuffer->indexToSeq(ackIndex));
			}
			stream->write<double64>(historyBuffer->getTime(currentSeqIndex));
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
				index = (historyBuffer->seqToIndex(peer->getLastAck()) + 1) % NSL_PACKET_BUFFER_SIZE_SERVER;
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
				stream->write<Attribute<seqNumber> >(historyBuffer->indexToSeq(currentSeqIndex));
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
		}

		void ProtocolParser::pushBufferedMessagesByIndex(BitStreamWriter* stream, int bufferIndex, Peer* peer)
		{
			std::vector<std::pair<byte*, unsigned int> > messages = peer->getBufferedCustomMessages(bufferIndex);
			if (!messages.empty()) {
				stream->write<Attribute<seqNumber> >(historyBuffer->indexToSeq(bufferIndex));
				for(std::vector<std::pair<byte*, unsigned int> >::iterator it = messages.begin(); it != messages.end(); it++) {
					stream->write<Attribute<customMessageSizeNumber> >(it->second);
					stream->writeRaw(it->second, it->first);
				}
				stream->write<Attribute<customMessageSizeNumber> >(0);
			}
		}

		void ProtocolParser::writeObjectData(ObjectClassDefinition* objectClass, BitStreamWriter* stream, byte* data)
		{
			for (unsigned int i = 0; i < objectClass->getAttributeCount(); i++) {
				int size = objectClass->getAttributeDefinition(i)->size;
				int offset = objectClass->getDataOffset(i);
				stream->write(size, data+offset);
			}
		}
	};
};
