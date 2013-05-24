/*
 * Copyright (C) 2013 Petr Smrcek
 * This file is originally a part of the Net Stalker Library
 * For conditions of distribution and use, see copyright notice in nsl.h
 */

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
#include "ProtocolParser.h"
#include "Connection.h"
#include "../../include/nslClient.h"
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

			double timeOverlap;	// difference between optimal application time and current client time
			double lastUpdateTime;

			CustomMessageBuffer customMessageBuffer;
			std::vector<std::pair<BitStreamWriter*, bool> > newCustomMessages;	// message stream and reliability are stored

			ProtocolParser protocolParser;
		public:
			ClientImpl(Client* userObject, unsigned short applicationId);

			~ClientImpl(void);

			void registerObjectClass(ObjectClassDefinition* objectClass);

			void open(const char* address, const char* port, const char* clientPort = NULL);

			void close(void);

			ClientState updateNetwork(double time = 0);

			void flushNetwork(void);

			/// Max size is NSL_MAX_CUSTOM_MESSAGE_SIZE
			BitStreamWriter* createCustomMessage(bool reliable);
		};
	};
};
