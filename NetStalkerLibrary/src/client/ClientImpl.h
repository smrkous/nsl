#pragma once

namespace nsl {
	class Client;
	class ObjectClassDefinition;

};

#include "../configuration.h"
#include "NetworkObject.h"
#include "HistoryBuffer.h"
#include "ObjectManager.h"
#include "CustomMessageBuffer.h"
#include "Connection.h"
#include <time.h>
#include <deque>

namespace nsl {
	namespace client {
		class ClientImpl
		{
		private:
			Connection connection;
			ConnectionState lastConnectionState;
			HistoryBuffer historyBuffer;
			ObjectManager objectManager;
			Client* userObject;
			byte buffer[MAX_PACKET_SIZE];	// buffer for reading incomming packets

			double timeOverlap;	// difference between optimal application time and current client time
			double lastUpdateTime;

			CustomMessageBuffer customMessageBuffer;
			std::vector<std::pair<BitStreamWriter*, bool> > newCustomMessages;	// message stream and reliability are stored

			/// Append all custom messages from given index
			void pushBufferedMessagesByIndex(BitStreamWriter* stream, int bufferIndex);

			double getTime(void);
			void proccessUpdatePacket(BitStreamReader* stream);

			/// proccess data of new object from stream
			/// if o == NULL, new object will be created and returned via reference
			/// otherwise data from stream will be used to update the given object
			void extractObjectFromStream(NetworkObject*& o, BitStreamReader* stream, int seqIndex, unsigned short classId, unsigned int objectId, ObjectSnapshotMeta snapshotMeta, bool birth, bool death);

			byte* extractObjectData(ObjectClassDefinition* objectClass, BitStreamReader* stream);
		public:
			ClientImpl(Client* userObject, unsigned short applicationId, unsigned short clientPort);

			~ClientImpl(void);

			void registerObjectClass(ObjectClassDefinition* objectClass);

			void open(unsigned short port, const char* address);

			void close(void);

			/// Returns true, if there is opened connection, otherwise tries to connect (and returns false for now)
			bool updateNetwork(void);

			void flushNetwork(void);

			/// Max size is NSL_MAX_CUSTOM_MESSAGE_SIZE
			BitStreamWriter* createCustomMessage(bool reliable);
		};
	};
};
