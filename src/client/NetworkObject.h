#pragma once
#include "../common.h"
#include "../ObjectClassDefinition.h"
#include "HistoryBuffer.h"
#include "../../include/nslClient.h"

namespace nsl {
	namespace client {

		enum ObjectState { BEFORE_BIRTH, ALIVE, SCHEDULED_DESTRUCTION, DESTROYED };

		class ObjectManager;

		class NetworkObject 
		{
		private:
			int id;								// unique identificator of object
			ObjectManager* objectManager;		// pointer to class, that handles objects
			ObjectClassDefinition* objectClass;	// definition of data field types
			byte* data[PACKET_BUFFER_SIZE];		// data for every seq of object existence
			ClientObject* clientObject;
			byte* interpolationPoints[NSL_INTERPOLATION_MINIMAL_DATA_COUNT];
			double pointTimes[NSL_INTERPOLATION_MINIMAL_DATA_COUNT];
			int interpolationPointsCount;
			double time;
		public:
			ObjectState state;
	
			NetworkObject(ObjectClassDefinition* objectClass, ObjectManager* objectManager, int id);
			~NetworkObject(void);
			int getId(void);
			void findInterpolationPoints(HistoryBuffer*, double); // int* interpolationPoints, int interpolationPointsCount
			ObjectClassDefinition* getObjectClass(void);
			byte* getDataBySeqIndex(int seqIndex);
			void setDataBySeqIndex(int seqIndex, byte* data);
			ClientObject* getClientObject(void);
			void beforeApplicationCreation(void);	// TODO: set object alive
			void afterApplicationDestruction(void);	// TODO: lock or delete client object
		};
	};
};

