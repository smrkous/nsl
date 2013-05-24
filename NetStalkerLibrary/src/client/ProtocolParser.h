/*
 * Copyright (C) 2013 Petr Smrcek
 * This file is originally a part of the Net Stalker Library
 * For conditions of distribution and use, see copyright notice in nsl.h
 */

#pragma once

namespace nsl {
	class ObjectClassDefinition;
	class BitStreamWriter;

	namespace client {
		class HistoryBuffer;
		class ObjectManager;
		class CustomMessageBuffer;
	};
};

#include <map>
#include <set>
#include "../configuration.h"
#include "NetworkObject.h"

namespace nsl {
	namespace client {
		class ProtocolParser
		{
		private:
			HistoryBuffer* historyBuffer;
			ObjectManager* objectManager;
			CustomMessageBuffer* customMessageBuffer;
		public:
			ProtocolParser(HistoryBuffer* historyBuffer, ObjectManager* objectManager, CustomMessageBuffer* customMessageBuffer);
			~ProtocolParser(void);

			/// Append all custom messages from given index
			void pushBufferedMessagesByIndex(BitStreamWriter* stream, int bufferIndex);

			void proccessUpdatePacket(BitStreamReader* stream, double applicationTime);

			/// proccess data of new object from stream
			/// if o == NULL, new object will be created and returned via reference
			/// otherwise data from stream will be used to update the given object
			void extractObjectFromStream(NetworkObject*& o, BitStreamReader* stream, int seqIndex, unsigned short classId, unsigned int objectId, ObjectSnapshotMeta snapshotMeta, bool birth, bool death);

			byte* extractObjectData(ObjectClassDefinition* objectClass, BitStreamReader* stream);
		};
	};
};