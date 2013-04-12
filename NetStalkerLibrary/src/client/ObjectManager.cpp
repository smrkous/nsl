#include "ObjectManager.h"
#include "../ObjectClassDefinition.h"
#include "NetworkObject.h"
#include "HistoryBuffer.h"
#include "../../include/nslClient.h"
#include <string.h>
#include <iostream>
#include <fstream>

namespace nsl {
	namespace client {

		ObjectManager::ObjectManager()
		{

		}

		ObjectManager::~ObjectManager()
		{

		}

		void ObjectManager::registerObjectClass(ObjectClassDefinition* objectClass)
		{
			objectClasses.insert(std::pair<unsigned int, ObjectClassDefinition*>(objectClass->getId(), objectClass));
		}

		void ObjectManager::applicationUpdate(int firstIndexToProccess, int lastIndexToProccess, Client* userObject, double time)
		{
			// iterate trough all objects and set their interpolation points
			for (std::map<unsigned int, NetworkObject*>::iterator it = objectsBegin(); it != objectsEnd(); it++) {
				it->second->findInterpolationPoints(historyBuffer, lastIndexToProccess, time);
			}

			if (firstIndexToProccess != NSL_UNDEFINED_BUFFER_INDEX) {
				int previousIndex = historyBuffer->getPreviousValidIndex(firstIndexToProccess);
				int currentIndex = firstIndexToProccess;

				// iterate trough all valid indexes, which passed before the given time and manage scheduled operations
				while (true) {
					for (std::map<unsigned int, NetworkObject*>::iterator it = objectsBegin(); it != objectsEnd(); it++) {
						
						ObjectSnapshotMeta currentState = it->second->getStateBySeqIndex(currentIndex);
						// if there are no data for this object in this seq, no scheduled operations are about to happen
						if (currentState == ObjectSnapshotMeta::EMPTY) {
							continue;
						}
						
						ObjectSnapshotMeta previousState = (
							previousIndex == NSL_UNDEFINED_BUFFER_INDEX ? 
							ObjectSnapshotMeta::EMPTY : 
							it->second->getStateBySeqIndex(previousIndex)
						);
						
						bool create = false;
						bool destroy = false;
						// death / birth zalezi vzdy na meta

						switch (previousState) {
						case ObjectSnapshotMeta::UPDATED:
						case ObjectSnapshotMeta::CREATED:
							if (currentState == ObjectSnapshotMeta::DESTROYED || 
								currentState == ObjectSnapshotMeta::CREATED_AND_DESTROYED ||
								currentState == ObjectSnapshotMeta::EMPTY) {
								destroy = true;
							}
							break;
						case ObjectSnapshotMeta::DESTROYED:
						case ObjectSnapshotMeta::EMPTY:
							// alias updated, created or created_and_destroyed
							if (currentState != ObjectSnapshotMeta::DESTROYED && currentState != ObjectSnapshotMeta::EMPTY) {
								create = true;
							}
							if (currentState == ObjectSnapshotMeta::CREATED_AND_DESTROYED) {
								destroy = true;
							}
							break;
						case ObjectSnapshotMeta::CREATED_AND_DESTROYED:
							if (currentState == ObjectSnapshotMeta::CREATED || ObjectSnapshotMeta::UPDATED) {
								create = true;
							}
							break;
						}

						if (create) {
							it->second->beforeApplicationCreate();
							userObject->onCreate(it->second->getClientObject(), it->second->getBirthBySeqIndex(currentIndex));
						}

						if (destroy) {
							userObject->onDestroy(it->second->getClientObject(), it->second->getDeathBySeqIndex(currentIndex));
							it->second->afterApplicationDestroy();
						}
					}

					if (firstIndexToProccess == lastIndexToProccess) {
						break;
					}

					previousIndex = firstIndexToProccess;
					firstIndexToProccess = historyBuffer->getNextValidIndex(firstIndexToProccess);
				}				
			}
		}

		void ObjectManager::clearBufferIndex(int bufferIndex)
		{
			// delete packet object order info
			objectsInPacket[bufferIndex].clear();

			// objects in packet cannot be used, because during deletion in packet data are received but object is not added into packet
			for (std::map<unsigned int, NetworkObject*>::iterator it = objectsBegin(); it != objectsEnd(); it++) {

				NetworkObject* o = it->second;
				byte* data = o->getDataBySeqIndex(bufferIndex);

				if (data != NULL) {
					delete[] data;
					o->setDataBySeqIndex(bufferIndex, NULL, ObjectSnapshotMeta::EMPTY);
				}
			}
		}

		void ObjectManager::deleteOldObjects(void)
		{
			int firstNeccessaryIndex = historyBuffer->getFirstNeccessaryIndex();

			std::map<unsigned int, NetworkObject*>::iterator it = objectsBegin();
			while (it != objectsEnd()) {

				bool notUseful = false;
				int currentIndex = historyBuffer->getLastSeqIndex();

				do {
					ObjectSnapshotMeta state = it->second->getStateBySeqIndex(currentIndex);
					if (state != ObjectSnapshotMeta::EMPTY) {
						notUseful = false;
						break;
					}
				} while(currentIndex != firstNeccessaryIndex);

				if (notUseful) {
					NetworkObject* o = it->second;
					it = objects.erase(it);
					delete o;
				}
			}
		}

		NetworkObject* ObjectManager::createObject(unsigned short classId, unsigned int id)
		{
			std::map<unsigned short, ObjectClassDefinition*>::iterator it = objectClasses.find(classId);
			if (it == objectClasses.end()) {
				throw Exception(NSL_EXCEPTION_USAGE_ERROR, "NSL: trying to create unknown object");
			}

			NetworkObject* o = new NetworkObject(it->second, this, id);

			objects.insert(std::pair<unsigned int,NetworkObject*>(o->getId(),o));
			return o;
		} 

		NetworkObject* ObjectManager::findObjectById(unsigned int objectId)
		{
			std::map<unsigned int, NetworkObject*>::iterator it = objects.find(objectId);
			if (it == objects.end()) {
				return NULL;
			} else {
				return it->second;
			}
		}

		void ObjectManager::addObjectToPacket(int bufferIndex, NetworkObject* object)
		{
			objectsInPacket[bufferIndex].push_back(object);
		}

	};
};