#include "ObjectManager.h"
#include <string.h>
#include <iostream>
#include <fstream>

namespace nsl {
	namespace client {

		ObjectManager::ObjectManager()
		{
			lastAppUpdateIndex = UNDEFINED_BUFFER_INDEX;
			latestAckIndex = UNDEFINED_BUFFER_INDEX;
		}

		void ObjectManager::registerObjectClass(ObjectClassDefinition* objectClass)
		{
			objectClasses.insert(std::pair<short, ObjectClassDefinition*>(objectClass->getId(), objectClass);
		}

		int ObjectManager::getLastAppUpdateIndex(void)
		{
			return lastAppUpdateIndex;
		}

		void ObjectManager::applicationUpdate(double time, Client* userObject)
		{
			// first index, that should be examined
			int index;
			
			// if index is not initialized, try to find first index, set it and start from it
			if (lastAppUpdateIndex == UNDEFINED_BUFFER_INDEX) {
				lastAppUpdateIndex = historyBuffer->getFirstSeqIndex();
				if (lastAppUpdateIndex == UNDEFINED_BUFFER_INDEX) {
					throw Exception(NSL_EXCEPTION_LIBRARY_ERROR, "NSL: index for the first data could not be found, application cannot start");
				}

				index = lastAppUpdateIndex;
			} else {
				// otherwise start from the next index
				index = historyBuffer->getNextValidIndex(lastAppUpdateIndex);
			}

			// get target index and set history buffer to correct time
			lastAppUpdateIndex = historyBuffer->updateApplicationIndex(time);
			if (lastAppUpdateIndex == UNDEFINED_BUFFER_INDEX) {
				throw Exception(NSL_EXCEPTION_LIBRARY_ERROR, "NSL: data for required time are not present (probably already overriden)");
			}

			// iterate trough all valid indexes, which passed before the given time and manage scheduled operations
			while (index != UNDEFINED_BUFFER_INDEX) {
				for (std::vector<NetworkObject*>::iterator i = objectsByCreationSeq[index].begin(); i < objectsByCreationSeq[index].end(); i++) {
					(*i)->beforeApplicationCreation();
					userObject->onCreate((*i)->getClientObject());
				}
				for (std::vector<NetworkObject*>::iterator i = objectsByDestructionSeq[index].begin(); i < objectsByDestructionSeq[index].end(); i++) {
					userObject->onDestroy((*i)->getClientObject());
					(*i)->afterApplicationDestruction();
				}
				if (index == lastAppUpdateIndex) {
					break;
				}
				index = historyBuffer->getNextValidIndex(index);
			}

			// iterate trough all objects and set their interpolation points
			for (std::map<int, NetworkObject*>::iterator it = objectsBegin(); it != objectsEnd(); it++) {
				it->second->findInterpolationPoints(historyBuffer);
			}
		}

		void ObjectManager::clearBufferIndex(int bufferIndex)
		{
			objectsByCreationSeq[bufferIndex].clear();

			// all objects, that were destroyed in this index, will be deleted from memmory
			for (std::vector<NetworkObject*>::iterator it = objectsByDestructionSeq[bufferIndex].begin(); it < objectsByDestructionSeq[bufferIndex].end();) {
				NetworkObject* o = (*it);
				objects.erase(objects.find(o->getId()));
			}
			objectsByDestructionSeq[bufferIndex].clear();
		}

		NetworkObject* ObjectManager::createObject(int currentBufferIndex, short typeCode, int id)
		{
			std::map<short, ObjectClassDefinition*>::iterator it = objectClasses.find(typeCode);
			if (it == objectClasses.end()) {
				throw Exception(NSL_EXCEPTION_USAGE_ERROR, "NSL: trying to create unknown object");
			}

			NetworkObject* o = new NetworkObject(it->second, this, id);

			objects.insert(std::pair<int,NetworkObject*>(o->getId(),o));
			objectsByCreationSeq[currentBufferIndex].push_back(o);
			return o;
		}

		void ObjectManager::destroyObject(int currentBufferIndex, NetworkObject* o)
		{
			if (o->state == ALIVE) { 
				o->state = SCHEDULED_DESTRUCTION;
				objectsByDestructionSeq[currentBufferIndex].push_back(o);
			}
		}
	};
};