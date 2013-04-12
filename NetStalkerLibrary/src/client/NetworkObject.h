#pragma once

namespace nsl {
	class ObjectClassDefinition;
	class ClientObject;

	namespace client {
		class ObjectManager;
		class HistoryBuffer;
	};
};

#include "../common.h"
#include "../../include/nslReflexion.h"

namespace nsl {
	namespace client {

		enum ObjectSnapshotMeta {
			EMPTY,
			UPDATED,
			CREATED, 
			DESTROYED, 
			CREATED_AND_DESTROYED
		};

		class NetworkObject 
		{
		private:
			unsigned int id;					// unique identificator of object
			ObjectManager* objectManager;		// pointer to class, that handles objects
			ObjectClassDefinition* objectClass;	// definition of data field types

			byte* data[NSL_PACKET_BUFFER_SIZE];		// data for every seq of object existence
			ObjectSnapshotMeta state[NSL_PACKET_BUFFER_SIZE];
			bool birth[NSL_PACKET_BUFFER_SIZE];
			bool death[NSL_PACKET_BUFFER_SIZE];

			ClientObject* clientObject;

			byte* interpolationPoints[NSL_INTERPOLATION_MINIMAL_DATA_COUNT * 2 + 1];
			byte* attributeInPoints[NSL_PACKET_BUFFER_SIZE];		// tmp array for returning concrete attribute, data are shifted by offset
			double pointTimes[NSL_INTERPOLATION_MINIMAL_DATA_COUNT * 2 + 1];
			unsigned int interpolationPointsCount;
			unsigned int interpolationPointsOffset;

			double time;	// current application time TODO: this is duplicated in all objects

		public:
			NetworkObject(ObjectClassDefinition* objectClass, ObjectManager* objectManager, unsigned int id);
			~NetworkObject(void);

			/// get unique instance id
			unsigned int getId(void);

			/// find interpolation points for this object by time
			void findInterpolationPoints(HistoryBuffer*, int applicationIndex, double);

			/// get object class
			ObjectClassDefinition* getObjectClass(void);

			/// getter on attribute data
			/// if the value is not interpolated, false is returned and the single data piece is stored in data[0]
			bool get(
				unsigned int attrId, 
				unsigned int byteSize, 
				unsigned int& pointCount,
				byte**& data,
				double*& times,
				double& targetTime,
				abstractInterpolationFunction& interpolationFunction
			);

			byte* getDataBySeqIndex(int seqIndex);
			ObjectSnapshotMeta getStateBySeqIndex(int seqIndex);
			bool getBirthBySeqIndex(int seqIndex);
			bool getDeathBySeqIndex(int seqIndex);
			void setDataBySeqIndex(int seqIndex, byte* data, ObjectSnapshotMeta state, bool birth = false, bool death = false);

			/// get object which communicates with the application
			ClientObject* getClientObject(void);

			/// must be called right before object is presented to the application as created
			void beforeApplicationCreate(void);

			/// must be called right after object was presented to the application as destroyed
			void afterApplicationDestroy(void);
		};
	};
};

