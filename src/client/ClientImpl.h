#pragma once

#include "../common.h"
#include "../socket.h"
#include "../../include/nslClient.h"
#include "../ObjectClassDefinition.h"
#include "HistoryBuffer.h"
#include "ObjectManager.h"
#include "Connection.h"
#include <time.h>
#include <deque>

#define NSL_OBJECT_FLAG_DIFF 0
#define NSL_OBJECT_FLAG_DELETE 1
#define NSL_OBJECT_FLAG_SNAPSHOT 2
#define NSL_OBJECT_FLAG_NO_CHANGE 3

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
			std::deque<Message*> networkBuffer;
			double timeOffset;
			byte buffer[MAX_PACKET_SIZE];	// buffer for reading incomming packets
			unsigned short lastAck;		// last ack from server
			bool lastAckValid;	// was last ack already set or is undefined?
			

			double getTime(void);
			void proccessUpdatePacket(BitStreamReader* stream);
			byte* extractObjectData(ObjectClassDefinition* objectClass, BitStreamReader* stream);
		public:
			ClientImpl(Client* userObject, short applicationId, short clientPort);

			~ClientImpl(void);

			void registerObjectClass(ObjectClassDefinition* objectClass);

			void open(int port, const char* address) throw (Exception);

			void close(void);

			/// Returns true, if there is opened connection, otherwise tries to connect (and returns false for now)
			bool updateNetwork(void);

			void flushNetwork(void);

			BitStreamWriter* createCustomMessage(void);
		};
	};
};