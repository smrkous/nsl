#include <algorithm>
#include <cmath>
#include "ClientImpl.h"
#include "../../include/nslBitStream.h"
#include "NetworkObject.h"
#include "../ObjectClassDefinition.h"
using namespace std;

namespace nsl {
	namespace client {

		ClientImpl::ClientImpl(Client* userObject, unsigned short applicationId, unsigned short clientPort)
			: connection(applicationId, clientPort), objectManager(&historyBuffer)
		{
			this->userObject = userObject;
			lastConnectionState = CLOSED;
		}

		ClientImpl::~ClientImpl(void)
		{
			close();
		}

		double ClientImpl::getTime(void)
		{
			return ((double)clock())/CLOCKS_PER_SEC;
		}

		void ClientImpl::close(void) 
		{
			connection.close();
			lastConnectionState = CLOSED;
		}

		void ClientImpl::open(unsigned short port, const char* address)
		{
			connection.open(address, port, getTime());
		}

		void ClientImpl::registerObjectClass(ObjectClassDefinition* objectClass)
		{
			if (lastConnectionState != CLOSED) {
				throw Exception(NSL_EXCEPTION_USAGE_ERROR, "NSL: trying to register object class when connection already opened");
			}
			objectManager.registerObjectClass(objectClass);
		}

		bool ClientImpl::updateNetwork(void)
		{
			double currentTime = getTime();

			// check connection status and try to connect / handshake, if neccessary
			lastConnectionState = connection.update(currentTime);
			if (lastConnectionState == CLOSED) {
				throw Exception(NSL_EXCEPTION_DISCONNECTED, "NSL: connection closed.");
			}
			
			if (lastConnectionState != CONNECTED) {
				return false;
			}

			// proccess all incomming messages
			BitStreamReader* stream = connection.receive();
			try {
				while (stream != NULL) {
					proccessUpdatePacket(stream);
					stream = connection.receive();
				}
			} catch (Exception e) {
				if (e.getCode() == NSL_EXCEPTION_DISCONNECTED) {
					connection.close();
					lastConnectionState = CLOSED;
				}
				throw e;
			}

			// update application (if successful amount of data already arrived)
			if (historyBuffer.getValidUpdatesCount() >= NSL_MINIMAL_PACKET_COUNT) {

				int currentApplicationIndex = historyBuffer.getApplicationIndex();

				// count optimal application time
				double averageTickDuration = historyBuffer.getAverageTimeInterval(NSL_TIME_INTERVAL_AVERAGE_COUNT);
				double optimalApplicationTime = historyBuffer.getTime(historyBuffer.getLastSeqIndex()) - NSL_INTERPOLATION_LATENCY_PACKET_COUNT * averageTickDuration;

				// if this is first application update, find first index to start and set first timeOverlap
				bool isFirst = false;
				if (currentApplicationIndex == NSL_UNDEFINED_BUFFER_INDEX) {
					currentApplicationIndex = historyBuffer.getFirstSeqIndex();
					if (currentApplicationIndex == NSL_UNDEFINED_BUFFER_INDEX) {
						throw Exception(NSL_EXCEPTION_LIBRARY_ERROR, "NSL: application first update failed, first data snapshot was not found");
					}

					timeOverlap = optimalApplicationTime - currentTime;
					isFirst = true;

				} else {
					// count the difference between optimal timeOverlap and current timeOverlap
					double timeOverlapDiff = optimalApplicationTime - currentTime - timeOverlap;

					// limit the difference by maximal speedup / slowdown
					double step = min(std::abs(timeOverlapDiff), (currentTime - lastUpdateTime) * (1 - NSL_MAXIMAL_SPEEDUP));

					// alter the timeOverlap
					timeOverlap += (timeOverlap < 0) ? -step : step;
				}

				double updateTime = currentTime + timeOverlap;

				if (!historyBuffer.updateApplicationIndex(updateTime)) {
					throw Exception(NSL_EXCEPTION_LIBRARY_ERROR, "NSL: application time is no longer in buffer");
				}

				int targetApplicationIndex = historyBuffer.getApplicationIndex();

				// if this is not first update, index must be shifted, because current index has been already proccessed in previous update
				if (!isFirst) {
					if (currentApplicationIndex == targetApplicationIndex) {
						currentApplicationIndex = NSL_UNDEFINED_BUFFER_INDEX;
					} else {
						currentApplicationIndex = historyBuffer.getNextValidIndex(currentApplicationIndex);
					}
				}

				objectManager.applicationUpdate(currentApplicationIndex, targetApplicationIndex, userObject, updateTime);

				objectManager.deleteOldObjects();	// TODO: more effective would be calling this before applicationUpdate
			}
			
			// check state once more
			// disconnect could have occured during proccessing incomming messages
			lastConnectionState = connection.update(currentTime);
			if (lastConnectionState == CLOSED) {
				throw Exception(NSL_EXCEPTION_DISCONNECTED, "NSL: not connected.");
			}

			lastUpdateTime = currentTime;

			return true;
		}

		void ClientImpl::flushNetwork(void)
		{
			if (!historyBuffer.isEmpty()) {
				Packet* packet = connection.createPacket();
				BitStreamWriter* stream = packet->getStream();
				stream->write<Attribute<seqNumber> >(historyBuffer.indexToSeq(historyBuffer.getLastSeqIndex()));

				// custom message part
				customMessageBuffer.addSeq();
				int currentIndex = customMessageBuffer.getCurrentSeqIndex();
				stream->write<Attribute<seqNumber> >(customMessageBuffer.indexToSeq(currentIndex));

				// add unacked reliable custom messages
				int index = customMessageBuffer.getFirstUnackedIndex();
				
				while (index != currentIndex) {
					pushBufferedMessagesByIndex(stream, index);
					index = customMessageBuffer.getNextValidIndex(index);
				}

				// new messages (current index)
				if (!newCustomMessages.empty()) {
					index = customMessageBuffer.getCurrentSeqIndex();
					stream->write<Attribute<seqNumber> >(customMessageBuffer.indexToSeq(index));
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
							customMessageBuffer.addBufferedMessage(data, size);
						}

						delete it->first;
					}
					newCustomMessages.clear();
					stream->write<Attribute<customMessageSizeNumber> >(0);
				}

				packet->send();
			}
		}

		void ClientImpl::pushBufferedMessagesByIndex(BitStreamWriter* stream, int bufferIndex)
		{
			if (!customMessageBuffer.isIndexEmpty(bufferIndex)) {
				stream->write<Attribute<seqNumber> >(customMessageBuffer.indexToSeq(bufferIndex));
				for(std::vector<std::pair<byte*, unsigned int> >::iterator it = customMessageBuffer.bufferedMessagesBegin(bufferIndex); it != customMessageBuffer.bufferedMessagesEnd(bufferIndex); it++) {
					stream->write<Attribute<customMessageSizeNumber> >(it->second);
					stream->write(it->second, it->first);
				}
				stream->write<Attribute<customMessageSizeNumber> >(0);
			}
		}

		BitStreamWriter* ClientImpl::createCustomMessage(bool reliable)
		{
			BitStreamWriter* stream = new BitStreamWriter();
			newCustomMessages.push_back(std::pair<BitStreamWriter*, bool>(stream, reliable));
			return stream;
		}

		void ClientImpl::proccessUpdatePacket(BitStreamReader* stream)
		{
			seqNumber seq = stream->read<Attribute<seqNumber> >();
			seqNumber ack = stream->read<Attribute<seqNumber> >();
			double time = stream->read<double64>();

			// check seq validity, if ok, push seq 
			// object manager will clear invalidated indexes 
			// (but data from not deleted objects must be deleted manualy)
			if (!historyBuffer.pushSeq(seq, ack, time, &objectManager))
			{
				return;
			}
#ifdef NSL_LOG_PACKETS
					byte x = 99;
					logBytes(&x, 1);
#endif
			customMessageBuffer.updateAck(stream->read<Attribute<seqNumber> >());

			int seqIndex = historyBuffer.seqToIndex(seq);
			int ackIndex = historyBuffer.seqToIndex(ack);


			////////////////////// object modification part ////////////////////////

			for (std::vector<NetworkObject*>::iterator it = objectManager.objectsInPacketBegin(ackIndex);
				it != objectManager.objectsInPacketEnd(ackIndex); it++) {

				NetworkObject* object = *it;

				byte* ackData = object->getDataBySeqIndex(ackIndex);
				byte flag = stream->read<uint8>();
				ObjectClassDefinition* objectClass = object->getObjectClass();
				byte* newData;

				switch(flag) {
				case NSL_OBJECT_FLAG_DIFF:
					// standard object delivery, undo diff and store data
					newData = extractObjectData(objectClass, stream);

					for (unsigned int i = 0; i < objectClass->getByteSize(); i++) {
						newData[i] ^= ackData[i];
					}

					objectManager.addObjectToPacket(seqIndex, object);
					object->setDataBySeqIndex(seqIndex, newData, UPDATED);
					break;

				case NSL_OBJECT_FLAG_DELETE:
					// object was destroyed
					newData = extractObjectData(objectClass, stream);

					for (unsigned int i = 0; i < objectClass->getByteSize(); i++) {
						newData[i] ^= ackData[i];
					}
					
					object->setDataBySeqIndex(seqIndex, newData, DESTROYED);
					break;

				case NSL_OBJECT_FLAG_OUT_OF_SCOPE:
					// object was destroyed
					newData = extractObjectData(objectClass, stream);

					for (unsigned int i = 0; i < objectClass->getByteSize(); i++) {
						newData[i] ^= ackData[i];
					}
					
					object->setDataBySeqIndex(seqIndex, newData, DESTROYED, false, true);
					break;

				case NSL_OBJECT_FLAG_SNAPSHOT:
					// snapshot object delivery, store data
					newData = extractObjectData(objectClass, stream);

					objectManager.addObjectToPacket(seqIndex, object);
					object->setDataBySeqIndex(seqIndex, newData, UPDATED);
					break;
				/*
				case NSL_OBJECT_FLAG_NO_CHANGE:
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
				byte flag = stream->read<uint8>();

				if (flag == NSL_OBJECT_FLAG_END_OF_SECTION) {
					break;
				}

				unsigned short classId = stream->read<uint16>();
				unsigned int objectId = stream->read<uint32>();

				// object could have been already created, but server was not noticed yet, so it sent its data as a new object
				NetworkObject* o = objectManager.findObjectById(objectId);

				switch (flag) {
				case NSL_OBJECT_FLAG_CREATE:
					extractObjectFromStream(o, stream, seqIndex, classId, objectId, CREATED, true, false);
					objectManager.addObjectToPacket(seqIndex, o);
					break;

				case NSL_OBJECT_FLAG_CREATE_AND_DELETE:
					extractObjectFromStream(o, stream, seqIndex, classId, objectId, CREATED_AND_DESTROYED, true, true);
					break;

				case NSL_OBJECT_FLAG_CREATE_AND_OUT_OF_SCOPE:
					extractObjectFromStream(o, stream, seqIndex, classId, objectId, CREATED_AND_DESTROYED, true, false);
					break;

				case NSL_OBJECT_FLAG_IN_SCOPE:
					extractObjectFromStream(o,stream, seqIndex, classId, objectId, CREATED, false, false);
					objectManager.addObjectToPacket(seqIndex, o);
					break;

				case NSL_OBJECT_FLAG_IN_SCOPE_AND_DELETE:
					extractObjectFromStream(o, stream, seqIndex, classId, objectId, CREATED_AND_DESTROYED, false, true);
					break;

				case NSL_OBJECT_FLAG_IN_SCOPE_AND_OUT_OF_SCOPE:
					extractObjectFromStream(o, stream, seqIndex, classId, objectId, CREATED_AND_DESTROYED, false, false);
					break;

				default:
					throw Exception(NSL_EXCEPTION_LIBRARY_ERROR, "NSL: unknown flag during object creation");
				}
			}


			////////////////////// custom messages part ////////////////////////
		}

		byte* ClientImpl::extractObjectData(ObjectClassDefinition* objectClass, BitStreamReader* stream)
		{
			byte* data = new byte[objectClass->getByteSize()];

			for (unsigned int i = 0; i < objectClass->getAttributeCount(); i++) {
				int size = objectClass->getAttributeDefinition(i)->size;
				int offset = objectClass->getDataOffset(i);
				stream->read(size, data+offset);
			}

			return data;
		}

		void ClientImpl::extractObjectFromStream(NetworkObject*& o, BitStreamReader* stream, int seqIndex, unsigned short classId, unsigned int objectId, ObjectSnapshotMeta snapshotMeta, bool birth, bool death) 
		{
			byte* data;

			if (o == NULL) {
				o = objectManager.createObject(classId, objectId);
			}

			data = extractObjectData(o->getObjectClass(), stream);
			o->setDataBySeqIndex(seqIndex, data, snapshotMeta, birth, death);
		}

		
	};
};
