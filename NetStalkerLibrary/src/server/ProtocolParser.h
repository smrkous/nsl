/*
 * Copyright (C) 2013 Petr Smrcek
 * This file is originally a part of the Net Stalker Library
 * For conditions of distribution and use, see copyright notice in nsl.h
 */

#pragma once

namespace nsl {
	class ObjectClassDefinition;
	class BitStreamWriter;

	namespace server {
		class Peer;
		class HistoryBuffer;
		class NetworkObject;
	};
};

#include <map>
#include <set>
#include "../configuration.h"

namespace nsl {
	namespace server {
		class ProtocolParser
		{
		private:
			HistoryBuffer* historyBuffer;
		public:
			ProtocolParser(HistoryBuffer* historyBuffer);
			~ProtocolParser(void);

			/// Append all custom messages from given index
			void pushBufferedMessagesByIndex(BitStreamWriter* stream, int bufferIndex, Peer* peer);
			void writeUpdateToPeer(BitStreamWriter* stream, Peer* peer, std::set<NetworkObject*>& scope, int ackIndex);
			void writeObjectData(ObjectClassDefinition* objectClass, BitStreamWriter* stream, byte* data);
		};
	};
};