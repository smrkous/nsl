#pragma once

namespace nsl {
	class ObjectClassDefinition;
	class ServerObject;

	namespace server {
		class ObjectManager;
		class HistoryBuffer;
	};
};

#include "../configuration.h"

namespace nsl {
	namespace server {

		class NetworkObject
		{
		private:
			HistoryBuffer* historyBuffer;				// pointer to class, that handles objects
			ObjectClassDefinition* objectClass;			// definition of data field types
			byte* data[NSL_PACKET_BUFFER_SIZE_SERVER];	// data for every seq of object existence
			int destroyIndex;
			int creationIndex;
			unsigned int id;
			ServerObject* serverObject;
			byte* creationCustomMessage;
			unsigned int creationCustomMessageSize;
		public:
			NetworkObject(
				ObjectClassDefinition* objectClass, 
				HistoryBuffer* historyBuffer, 
				unsigned int id);
			~NetworkObject(void);
			unsigned int getId(void);
			ServerObject* getUserObject(void);
			void destroy(void);
			int getDestroyIndex(void);
			int getCreationIndex(void);
			void invalidateCreationIndex(void);
			ObjectClassDefinition* getObjectClass(void);
			void set(unsigned int attrId, unsigned int byteSize, byte* value);
			byte* getDataBySeqIndex(short seqIndex);
			void setDataBySeqIndex(short seqIndex, byte* data);
			void setCreationCustomMessage(byte* data, unsigned int size);
			bool getCreationCustomMessage(byte*& data, unsigned int& size);
		};
	};
};
