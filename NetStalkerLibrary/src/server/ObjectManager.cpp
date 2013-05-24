/*
 * Copyright (C) 2013 Petr Smrcek
 * This file is originally a part of the Net Stalker Library
 * For conditions of distribution and use, see copyright notice in nsl.h
 */

#include "ObjectManager.h"
#include "../ObjectClassDefinition.h"
#include "NetworkObject.h"
#include <string.h>

namespace nsl {
	namespace server {

		ObjectManager::ObjectManager(void)
		{
			lastId = 0;
		}

		ObjectManager::~ObjectManager(void)
		{

		}

		void ObjectManager::registerObjectClass(ObjectClassDefinition* objectClass)
		{
			objectClasses.insert(std::pair<unsigned int, ObjectClassDefinition*>(objectClass->getId(), objectClass));
		}

		void ObjectManager::clearBufferIndex(int bufferIndex, int defaultsBufferIndex)
		{
			std::map<unsigned int, NetworkObject*>::iterator it = objectsBegin();
			// objects in packet cannot be used, because during deletion in packet data are received but object is not added into packet
			while ( it != objectsEnd()) {

				NetworkObject* o = it->second;
				byte* data = o->getDataBySeqIndex(bufferIndex);

				// if object contains no more data, delete it from memmory
				if (o->getDestroyIndex() == bufferIndex) {
					objects.erase(it++);
					delete o;
					continue;
				}

				// invalidate creation index
				if (o->getCreationIndex() == bufferIndex) {
					o->invalidateCreationIndex();
				}

				// choose to copy default data (only if object was not deleted yet) or set NULL
				if (defaultsBufferIndex != NSL_UNDEFINED_BUFFER_INDEX) {
					byte* defaultData = o->getDataBySeqIndex(defaultsBufferIndex);
					if (defaultData != NULL) {
						if (data == NULL) {
							data = new byte[o->getObjectClass()->getByteSize()];
							o->setDataBySeqIndex(bufferIndex, data);
						}
						memcpy(data, defaultData, o->getObjectClass()->getByteSize());
					} else {
						if (data != NULL) {
							delete[] data;
							o->setDataBySeqIndex(bufferIndex, NULL);
						}
					}
				} else {
					if (data == NULL) {
						data = new byte[o->getObjectClass()->getByteSize()];
						o->setDataBySeqIndex(bufferIndex, data);
					}
					memset(data, 0, o->getObjectClass()->getByteSize());
				}

				it++;
			}
		}

		NetworkObject* ObjectManager::createObject(unsigned short classId, HistoryBuffer* historyBuffer)
		{
			std::map<unsigned short, ObjectClassDefinition*>::iterator it = objectClasses.find(classId);
			if (it == objectClasses.end()) {
				throw Exception(NSL_EXCEPTION_USAGE_ERROR, "NSL: trying to create unknown object");
			}

			NetworkObject* o = new NetworkObject(it->second, historyBuffer, ++lastId);

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
	};
};
