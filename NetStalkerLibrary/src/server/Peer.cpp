#include "Peer.h"
#include "Connection.h"
#include "../../include/nslBitStream.h"

namespace nsl {
	namespace server {
		Peer::Peer(PeerConnection* peer) : peerConnection(peer)
		{
			customMessageSeq = 0;
			isAck = false;
			firstUpdateIndex = NSL_UNDEFINED_BUFFER_INDEX;
		}

		Peer::~Peer(void)
		{
			// TODO	where to delete this struct?
			delete peerConnection;
		}

		PeerConnection* Peer::getPeerConnection(void)
		{
			return peerConnection;
		}

		seqNumber Peer::getLastAck(void)
		{
			return lastAck;
		}

		void Peer::setLastAck(seqNumber ack)
		{
			lastAck = ack;
			isAck = true;
		}

		bool Peer::hasAck(void)
		{
			return isAck;
		}

		std::vector<NetworkObject*>* Peer::getScope(int bufferIndex)
		{
			return &scope[bufferIndex];
		}

		void Peer::clearIndex(int bufferIndex)
		{
			if (firstUpdateIndex == bufferIndex) {
				firstUpdateIndex = NSL_UNDEFINED_BUFFER_INDEX;
			}

			for (std::vector<std::pair<byte*, unsigned int> >::iterator it = customMessageBuffer[bufferIndex].begin(); it != customMessageBuffer[bufferIndex].end(); it++) {
				delete[] it->first;
			}

			customMessageBuffer[bufferIndex].clear();
		}
	};
};