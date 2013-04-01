#include "ClientImpl.h"

namespace nsl {
	namespace client {

		ClientImpl::ClientImpl(Client* userObject, short applicationId, short clientPort)
			: connection(applicationId, clientPort)
		{
			this->userObject = userObject;
			timeOffset = 0;
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

		void ClientImpl::open(int port, const char* address)
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

			// do the update logic
			if (lastAckValid) {
				// client can now receive its first objects, because:
				// server sent already at least 1 update besides the initial object creation in first packet (and its duplicities)
				// interpolation is available though and the last acquired data are freshest possible

				// find best client time (with interpolation latency)
				double serverTime = historyBuffer.getTime(historyBuffer.getLastSeqIndex());

				// TODO: nastaveni casu:
				// vezme se casovy rozdil mezi stavajicim seq a prechozim seq (takhle 3*, pokud jsou hodnoty a udela se prumer)
				// to je stavajici cas mezi snapshoty
				// umela latence by se mela pohybovat kolem 2.1*(cas mezi snapshoty)

				// pokud je jina, upravi se o min(doba od posledniho updateNetwork/10, rozdil mezi aktualni a optimalni latenci)

				// prvni nastaveni je treba provest az po 2 updatech, aby byl relevantni interval (a inicialni interval se nepouzije)
				
				// pak je treba porovnat stavajici cas se serverovym - pokud klient predbehne o moc, zastavit ho (lag), pokud o jedte vic, disconnect


				// zatim nahradni reseni (konstantni interpolacni rezerva 100ms):
				if (timeOffset == 0) {
					timeOffset = serverTime - currentTime - 0.1;
				}
				double time = currentTime + timeOffset;

				objectManager.applicationUpdate(time, userObject);
			}

			// check state once more
			// disconnect could have occured during proccessing incomming messages
			lastConnectionState = connection.update(currentTime);
			if (lastConnectionState == CLOSED) {
				throw Exception(NSL_EXCEPTION_DISCONNECTED, "NSL: not connected.");
			}

			return true;
		}

		void ClientImpl::flushNetwork(void)
		{
			// TODO: append all custom message streams
			// all streams should lock after the flush...
		}

		BitStreamWriter* ClientImpl::createCustomMessage(void)
		{
			// save the stream, it will be appended after flush
		}

		void ClientImpl::proccessUpdatePacket(BitStreamReader* stream)
		{
			short seq = stream->read<int16>();
			short ack = stream->read<int16>();
			double time = stream->read<double64>();

			// store previous last seq before push
			short oldSeqIndex = historyBuffer.getLastSeqIndex();

			// check seq validity, if ok, push seq 
			// object manager will clear invalidated indexes 
			// (but data from not deleted objects must be deleted manualy)
			if (!historyBuffer.pushSeq(seq, time, &objectManager))
			{
				return;
			}

			// check ack validity (if seq == ack, this is first packet)
			if (seq != ack && !historyBuffer.isSeqInBounds(ack)) {
				// packet cannot be decompressed, because ack data are not present
				throw Exception(NSL_EXCEPTION_DISCONNECTED, "NSL: ack data not present, packets cannot be decompressed anymore");
			}

			// set last server ack, if it is really last
			if (seq != ack && (!lastAckValid || historyBuffer.isSecondSeqGreater(lastAck, ack))) {
				lastAckValid = true;
				lastAck = ack;
			}

			int seqIndex = historyBuffer.seqToIndex(seq);
			int ackIndex = historyBuffer.seqToIndex(ack);

			////////////////////// object modification part ////////////////////////


			// iterate throught all objects present in client and update their values
			// if ack == seq, this packet is first, and there will be no objects yet, so the cycle won't run at all
			for (std::map<int,NetworkObject*>::iterator it = objectManager.objectsBegin(); it != objectManager.objectsEnd(); ++it ) {

				NetworkObject* object = it->second;

				// delete old data from target destination (for every object, because the seq has changed)
				byte* oldData = object->getDataBySeqIndex(seqIndex);
				if (oldData != NULL) {
					delete[] oldData;
					object->setDataBySeqIndex(seqIndex, NULL);
				}

				// if there are ack data in this object, packet contains update/delete information
				byte* ackData = object->getDataBySeqIndex(ackIndex);
				if (ackData != NULL) {
					byte flag = stream->read<uint2>();
					ObjectClassDefinition* objectClass = object->getObjectClass();

					switch(flag) {
					case NSL_OBJECT_FLAG_DIFF:
						// standard object delivery, undo diff and store data
						byte* newData = extractObjectData(objectClass, stream);

						for (int i = 0; i < objectClass->getByteSize(); i++) {
							newData[i] ^= ackData[i];
						}

						object->setDataBySeqIndex(seqIndex, newData);
						continue;

					case NSL_OBJECT_FLAG_DELETE:
						// object was destroyed
						objectManager.destroyObject(seqIndex, object);
						continue;

					case NSL_OBJECT_FLAG_SNAPSHOT:
						// snapshot object delivery, store data
						byte* newData = extractObjectData(objectClass, stream);

						object->setDataBySeqIndex(seqIndex, newData);
						continue;

					case NSL_OBJECT_FLAG_NO_CHANGE:
						// no change occured, just copy ack data to the current destination
						byte* newData = new byte[objectClass->getByteSize()];
						memcpy(newData, ackData, objectClass->getByteSize());
						continue;
					}
				}
			}	// end of object iteration


			////////////////////// object creation part ////////////////////////
			

			// number of remaining objects in this packet
			short newObjectCount = stream->read<uint16>();

			// new objects from this packet could have been created already, but server was not acked yet, 
			// so it has sent them again - use those data as updates (snapshots)			
			if (seq != ack) {
				int skippedSeqIndex = historyBuffer.getPreviousValidIndex(seqIndex);
				while (skippedSeqIndex != ackIndex) {
					if (skippedSeqIndex == UNDEFINED_BUFFER_INDEX) {
						throw Exception(NSL_EXCEPTION_LIBRARY_ERROR, "Ack index not met when iterating backwards from seq");
					}

					for (std::vector<NetworkObject*>::iterator it = objectManager.objectsByCreationSeqIndexBegin(skippedSeqIndex); it != objectManager.objectsByCreationSeqIndexEnd(skippedSeqIndex); it++) {
						ObjectClassDefinition* objectClass = (*it)->getObjectClass();

						// read object header
						byte flag = stream->read<uint2>();
						int classId = stream->read<uint16>();
						int objectId = stream->read<uint32>();

						// DEBUG
						if (objectClass->getId() != classId || (*it)->getId() != objectId) {
							throw new Exception(NSL_EXCEPTION_LIBRARY_ERROR, "NSL: protocol inconsistency - different object type expected");
						}

						// use data as snapshot
						byte* newData = extractObjectData(objectClass, stream);

						byte* oldData = (*it)->getDataBySeqIndex(skippedSeqIndex);
						if (oldData != NULL) {
							delete[] oldData;
						}

						(*it)->setDataBySeqIndex(skippedSeqIndex, newData);

						// if this object was created and deleted between seq and ack by server, 
						// but just created on client, delete it now
						if (flag == FLAG_CREATE_AND_DELETE) {
							objectManager.destroyObject(seqIndex, *it);
						}

						// data already used, decrement counter of remaining objects in this packet
						newObjectCount--;
					}

					skippedSeqIndex = historyBuffer.getPreviousValidIndex(skippedSeqIndex);
				} // end of iteration trought skipped seqs
			}

			// create new objects
			for(; newObjectCount > 0; newObjectCount--) {

				// read object header
				byte flag = stream->read<uint2>();
				short classId = stream->read<uint16>();
				int objectId = stream->read<uint32>();

				// DEBUG
				if (flag != FLAG_CREATE && flag != FLAG_CREATE_AND_DELETE) {
					throw new Exception(NSL_EXCEPTION_LIBRARY_ERROR, "NSL: protocol inconsistency - no known flag received during object creation");
				}

				NetworkObject* o = objectManager.createObject(seqIndex, classId, objectId);
				ObjectClassDefinition* objectClass = o->getObjectClass();
				byte* newData = extractObjectData(objectClass, stream);
				o->setDataBySeqIndex(seqIndex, newData);

				// if this object was created and deleted in one snapshot, delete it as well
				if (flag == FLAG_CREATE_AND_DELETE) {
					objectManager.destroyObject(seqIndex, o);
				}
			}
		}

		byte* ClientImpl::extractObjectData(ObjectClassDefinition* objectClass, BitStreamReader* stream) {
			byte* newData = new byte[objectClass->getByteSize()];

			for (int i = 0; i < objectClass->getAttributeCount(); i++) {
				int size = objectClass->getAttributeDefinition(i)->size;
				int offset = objectClass->getDataOffset(i);
				stream->read(size, newData+offset);
			}

			return newData;
		}

		////////////////////// custom messages part ////////////////////////
	}
}