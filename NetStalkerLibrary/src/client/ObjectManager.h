/*
 * Copyright (C) 2013 Petr Smrcek
 * This file is originally a part of the Net Stalker Library
 * For conditions of distribution and use, see copyright notice in nsl.h
 */

#pragma once

namespace nsl {
	class ObjectClassDefinition;
	class Client;

	namespace client {
		class HistoryBuffer;
		class NetworkObject;
	};
};

#include "../configuration.h"
#include <vector>
#include <map>
#include <iterator>

namespace nsl {
	namespace client {
		
		class ObjectManager 
		{
		private:
			HistoryBuffer* historyBuffer;
			std::map<unsigned int,NetworkObject*> objects;
			std::map<unsigned short, ObjectClassDefinition*> objectClasses;
			std::vector<NetworkObject*> objectsInPacket[NSL_PACKET_BUFFER_SIZE];
		public:
			ObjectManager(HistoryBuffer*);
			~ObjectManager(void);

			/// resets class to original state
			void reset(void);

			// manage scheduled actions
			// if firstIndexToProccess == NSL_UNDEFINED_BUFFER_INDEX, no indexes should be updated
			void applicationUpdate(int firstIndexToProccess, int lastIndexToProccess, Client* userObject, double time);		
			
			void registerObjectClass(ObjectClassDefinition* objectClass);

			/// wipe all data from that index (scheduled actions, object data, etc.) so it can be used to store new data
			void clearBufferIndex(int bufferIndex);

			/// deletes no longer useful objects from memmory
			void deleteOldObjects(void);

			/// create new object with no data
			NetworkObject* createObject(unsigned short classId, unsigned int id);

			const std::map<unsigned int,NetworkObject*>::iterator objectsBegin(void) {return objects.begin();}
			const std::map<unsigned int,NetworkObject*>::iterator objectsEnd(void) {return objects.end();}

			/// if no object is found, NULL is returned
			NetworkObject* findObjectById(unsigned int objectId);

			const std::vector<NetworkObject*>::iterator objectsInPacketBegin(int bufferIndex) {return objectsInPacket[bufferIndex].begin();}
			const std::vector<NetworkObject*>::iterator objectsInPacketEnd(int bufferIndex) {return objectsInPacket[bufferIndex].end();}
			void addObjectToPacket(int bufferIndex, NetworkObject* object);
		};
	};
};
