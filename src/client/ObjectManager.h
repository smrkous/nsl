#pragma once

#include "../common.h"
#include "HistoryBuffer.h"
#include "NetworkObject.h"
#include "../ObjectClassDefinition.h"
#include "../../include/nslClient.h"
#include <vector>
#include <map>
#include <iterator>

namespace nsl {
	namespace client {

		class NetworkObject;

		class ObjectManager 
		{
		private:
			HistoryBuffer* historyBuffer;
			int lastAppUpdateIndex;	// last index (in historyBuffer), for which application time already passed
			int latestAckIndex; // index of last ack (in history buffer) - older objects doesn't have to be stored anymore
			std::map<int,NetworkObject*> objects;
			std::vector<NetworkObject*> objectsByCreationSeq[PACKET_BUFFER_SIZE];
			std::vector<NetworkObject*> objectsByDestructionSeq[PACKET_BUFFER_SIZE];
			std::map<short, ObjectClassDefinition*> objectClasses;

		public:
			ObjectManager(void);
			~ObjectManager(void);
			void applicationUpdate(double time, Client* userObject);		// manage scheduled actions
			int getLastAppUpdateIndex(void); // get last index (in historyBuffer), whose time was already passed by application
			void registerObjectClass(ObjectClassDefinition* objectClass);

			void clearBufferIndex(int bufferIndex);
			NetworkObject* createObject(int currentBufferIndex, short typeCode, int id);
			void destroyObject(int currentBufferIndex, NetworkObject* object);	// schedules object destruction (only for the first time, the object is passed)

			const std::map<int,NetworkObject*>::iterator objectsBegin(void) {return objects.begin();}
			const std::map<int,NetworkObject*>::iterator objectsEnd(void) {return objects.end();}
			const std::vector<NetworkObject*>::iterator objectsByCreationSeqIndexBegin(int seqIndex) {return objectsByCreationSeq[seqIndex].begin();}
			const std::vector<NetworkObject*>::iterator objectsByCreationSeqIndexEnd(int seqIndex) {return objectsByCreationSeq[seqIndex].end();}
			const std::vector<NetworkObject*>::iterator objectsByDestructionSeqIndexBegin(int seqIndex) {return objectsByDestructionSeq[seqIndex].begin();}
			const std::vector<NetworkObject*>::iterator objectsByDestructionSeqIndexEnd(int seqIndex) {return objectsByDestructionSeq[seqIndex].end();}
		};
	};
};
