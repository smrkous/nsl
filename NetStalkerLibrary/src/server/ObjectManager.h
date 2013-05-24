/*
 * Copyright (C) 2013 Petr Smrcek
 * This file is originally a part of the Net Stalker Library
 * For conditions of distribution and use, see copyright notice in nsl.h
 */

#pragma once

namespace nsl {
	class ObjectClassDefinition;

	namespace server {
		class NetworkObject;
		class HistoryBuffer;
	};
};

#include "../configuration.h"
#include <vector>
#include <map>
#include <iterator>

namespace nsl {
	namespace server {

		class ObjectManager 
		{
		private:
			std::map<unsigned int,NetworkObject*> objects;
			std::map<unsigned short, ObjectClassDefinition*> objectClasses;
			unsigned int lastId;
		public:
			ObjectManager(void);
			~ObjectManager(void);
			
			void registerObjectClass(ObjectClassDefinition* objectClass);

			/// wipe all data from that index so it can be used to store new data
			/// if defaultsBufferIndex is set, data of all objects in this index will be set to copy of defaults
			/// if all data of some object are deleted, it is deleted from memmory
			void clearBufferIndex(int bufferIndex, int defaultsBufferIndex = NSL_UNDEFINED_BUFFER_INDEX);

			/// create new object with no data
			NetworkObject* createObject(unsigned short classId, HistoryBuffer* historyBuffer);

			const std::map<unsigned int,NetworkObject*>::iterator objectsBegin(void) {return objects.begin();}
			const std::map<unsigned int,NetworkObject*>::iterator objectsEnd(void) {return objects.end();}

			/// if no object is found, NULL is returned
			NetworkObject* findObjectById(unsigned int objectId);
		};
	};
};
