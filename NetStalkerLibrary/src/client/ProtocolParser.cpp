#include "ProtocolParser.h"
#include "HistoryBuffer.h"
#include "ObjectManager.h"
#include "CustomMessageBuffer.h"
#include "../ObjectClassDefinition.h"

namespace nsl {
	namespace client {
		ProtocolParser::ProtocolParser(HistoryBuffer* historyBuffer, ObjectManager* objectManager, CustomMessageBuffer* customMessageBuffer) 
			: historyBuffer(historyBuffer), objectManager(objectManager), customMessageBuffer(customMessageBuffer)
		{}

		ProtocolParser::~ProtocolParser(void)
		{}

		void ProtocolParser::pushBufferedMessagesByIndex(BitStreamWriter* stream, int bufferIndex)
		{
			if (!customMessageBuffer->isIndexEmpty(bufferIndex)) {
				stream->write<Attribute<seqNumber> >(customMessageBuffer->indexToSeq(bufferIndex));
				for(std::vector<std::pair<byte*, unsigned int> >::iterator it = customMessageBuffer->bufferedMessagesBegin(bufferIndex); it != customMessageBuffer->bufferedMessagesEnd(bufferIndex); it++) {
					stream->write<Attribute<customMessageSizeNumber> >(it->second);
					stream->write(it->second, it->first);
				}
				stream->write<Attribute<customMessageSizeNumber> >(0);
			}
		}

		void ProtocolParser::proccessUpdatePacket(BitStreamReader* stream)
		{
			seqNumber seq = stream->read<Attribute<seqNumber> >();
			seqNumber ack = stream->read<Attribute<seqNumber> >();
			double time = stream->read<double64>();

			// check seq validity, if ok, push seq 
			// object manager will clear invalidated indexes 
			// (but data from not deleted objects must be deleted manualy)
			int firstIndexToClear;
			int lastIndexToClear;
			if (!historyBuffer->pushSeq(seq, ack, time, firstIndexToClear, lastIndexToClear))
			{
				return;
			}

			// clear invalidated indexes
			if (firstIndexToClear != NSL_UNDEFINED_BUFFER_INDEX) {
				while (firstIndexToClear != lastIndexToClear) {
					objectManager->clearBufferIndex(firstIndexToClear);
					firstIndexToClear = historyBuffer->getNextIndex(firstIndexToClear);
				}
				objectManager->clearBufferIndex(lastIndexToClear);
			}

			customMessageBuffer->updateAck(stream->read<Attribute<seqNumber> >());

			int seqIndex = historyBuffer->seqToIndex(seq);
			int ackIndex = historyBuffer->seqToIndex(ack);


			////////////////////// object modification part ////////////////////////

			for (std::vector<NetworkObject*>::iterator it = objectManager->objectsInPacketBegin(ackIndex);
				it != objectManager->objectsInPacketEnd(ackIndex); it++) {

				NetworkObject* object = *it;

				byte* ackData = object->getDataBySeqIndex(ackIndex);
				ObjectFlags flags;
				*stream >> flags;
				ObjectClassDefinition* objectClass = object->getObjectClass();
				byte* newData;

				switch(flags.action) {
				case NSL_OBJECT_FLAG_ACTION_DIFF:
					// standard object delivery, undo diff and store data
					newData = extractObjectData(objectClass, stream);

					for (unsigned int i = 0; i < objectClass->getByteSize(); i++) {
						newData[i] ^= ackData[i];
					}

					objectManager->addObjectToPacket(seqIndex, object);
					object->setDataBySeqIndex(seqIndex, newData, UPDATED);
					break;

				case NSL_OBJECT_FLAG_ACTION_DELETE:
					// object was destroyed
					newData = extractObjectData(objectClass, stream);

					for (unsigned int i = 0; i < objectClass->getByteSize(); i++) {
						newData[i] ^= ackData[i];
					}
					
					if (flags.scopeDestroy == NSL_OBJECT_FLAG_SD_HIDE) {
						object->setDataBySeqIndex(seqIndex, newData, DESTROYED, false, true);
					} else {
						object->setDataBySeqIndex(seqIndex, newData, DESTROYED);
					}
					break;

				case NSL_OBJECT_FLAG_ACTION_SNAPSHOT:
					// snapshot object delivery, store data
					newData = extractObjectData(objectClass, stream);

					objectManager->addObjectToPacket(seqIndex, object);
					object->setDataBySeqIndex(seqIndex, newData, UPDATED);
					break;
				/*
				case NSL_OBJECT_FLAG_ACTION_NO_CHANGE:
					// no change occured, just copy ack data to the current destination
					newData = new byte[objectClass->getByteSize()];
					memcpy(newData, ackData, objectClass->getByteSize());

					objectManager.addObjectToPacket(seqIndex, object);
					object->setDataBySeqIndex(seqIndex, newData, UPDATED);
					break;
					*/
				}
				
			}	// end of object iteration


			////////////////////// object creation part ////////////////////////
			
			while (true) {
				ObjectFlags flags;
				*stream >> flags;

				if (flags.action == NSL_OBJECT_FLAG_ACTION_END_OF_SECTION) {
					break;
				}

				unsigned short classId = stream->read<uint16>();
				unsigned int objectId = stream->read<uint32>();

				// object could have been already created, but server was not noticed yet, so it sent its data as a new object
				NetworkObject* o = objectManager->findObjectById(objectId);

				// try to extract meta information for object creation
				BitStreamReader* creationReader = NULL;
				if (flags.creationCustomMessage == NSL_OBJECT_FLAG_CM_PRESENT) {
					unsigned int customMessageSize = stream->read<Attribute<customMessageSizeNumber> >();

					// extract message only if it has not arrived yet, otherwise skip it
					if (o == NULL || o->getCreationCustomMessage() == NULL) {
						creationReader = stream->createSubreader(customMessageSize, true);
					} 
					stream->skipBits(8*customMessageSize);
				}

				switch (flags.action) {
				case NSL_OBJECT_FLAG_ACTION_CREATE:
					extractObjectFromStream(o, stream, seqIndex, classId, objectId, 
						CREATED, flags.scopeCreate == NSL_OBJECT_FLAG_SC_BIRTH, false);
					objectManager->addObjectToPacket(seqIndex, o);
					break;

				case NSL_OBJECT_FLAG_ACTION_CREATE_AND_DELETE:
					extractObjectFromStream(o, stream, seqIndex, classId, objectId, 
						CREATED_AND_DESTROYED, flags.scopeCreate == NSL_OBJECT_FLAG_SC_BIRTH, flags.scopeCreate == NSL_OBJECT_FLAG_SD_DEATH);
					break;

				default:
					throw Exception(NSL_EXCEPTION_LIBRARY_ERROR, "NSL: unknown flag during object creation");
				}

				// if there were any meta creation data, add them to object
				if (creationReader != NULL) {
					o->setCreationCustomMessage(creationReader);
				}
			}


			////////////////////// custom messages part ////////////////////////
		}

		byte* ProtocolParser::extractObjectData(ObjectClassDefinition* objectClass, BitStreamReader* stream)
		{
			byte* data = new byte[objectClass->getByteSize()];

			for (unsigned int i = 0; i < objectClass->getAttributeCount(); i++) {
				int size = objectClass->getAttributeDefinition(i)->size;
				int offset = objectClass->getDataOffset(i);
				stream->read(size, data+offset);
			}

			return data;
		}

		void ProtocolParser::extractObjectFromStream(NetworkObject*& o, BitStreamReader* stream, int seqIndex, unsigned short classId, unsigned int objectId, ObjectSnapshotMeta snapshotMeta, bool birth, bool death) 
		{
			byte* data;

			if (o == NULL) {
				o = objectManager->createObject(classId, objectId);
			}

			data = extractObjectData(o->getObjectClass(), stream);
			o->setDataBySeqIndex(seqIndex, data, snapshotMeta, birth, death);
		}
	};
};
