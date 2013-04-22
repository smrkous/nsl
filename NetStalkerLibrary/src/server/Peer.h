#pragma once

namespace nsl {
	class BitStreamWriter;

	namespace server {
		struct PeerConnection;
		class NetworkObject;
	};
};

#include "../configuration.h"
#include "../../include/nslServer.h"
#include <vector>

namespace nsl {
	namespace server {

		class Peer {
		private:
			PeerConnection* peerConnection;
			std::vector<NetworkObject*> scope[NSL_PACKET_BUFFER_SIZE_SERVER];
			std::vector<std::pair<byte*, unsigned int> > customMessageBuffer[NSL_PACKET_BUFFER_SIZE_SERVER];
			std::vector<std::pair<BitStreamWriter*, bool> > newCustomMessages;
			seqNumber lastAck;
			seqNumber customMessageSeq;
			int firstUpdateIndex;
			bool isAck;
			nsl::Peer* userObject;
		public:
			Peer(PeerConnection* peer);
			~Peer(void);
			nsl::Peer* getUserObject(void);
			PeerConnection* getPeerConnection(void);
			seqNumber getCustomMessageSeq(void) {return customMessageSeq;}
			void setCustomMessageSeq(seqNumber seq) {customMessageSeq = seq;}
			seqNumber getLastAck(void);
			void setLastAck(seqNumber ack);
			bool hasAck(void);
			void clearIndex(int bufferIndex);
			int getFirstUpdateIndex(void) {return firstUpdateIndex;}
			void setFirstUpdateIndex(int bufferIndex) {firstUpdateIndex = bufferIndex;};
			std::vector<NetworkObject*>* getScope(int bufferIndex);
			std::vector<std::pair<byte*, unsigned int> >& getBufferedCustomMessages(int bufferIndex) {return customMessageBuffer[bufferIndex];}
			std::vector<std::pair<BitStreamWriter*, bool> >& getNewCustomMessages() {return newCustomMessages;}
		};
	};
};