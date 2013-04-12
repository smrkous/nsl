#include "HistoryBuffer.h"
#include "ObjectManager.h"
#include "Peer.h"

#include <algorithm>

namespace nsl {
	namespace server {

		HistoryBuffer::HistoryBuffer(void)
		{
			currentSeq = 0;
			validUpdatesCounter = 0;
		}

		HistoryBuffer::~HistoryBuffer(void)
		{
		}

		int HistoryBuffer::seqToIndex(seqNumber seq)
		{
			return (currentIndex - ((currentSeq - seq + NSL_SEQ_MODULO) % NSL_SEQ_MODULO) + NSL_PACKET_BUFFER_SIZE) % NSL_PACKET_BUFFER_SIZE;
		}

		seqNumber HistoryBuffer::indexToSeq(int index)
		{
			return (currentSeq - ((currentIndex - index + NSL_PACKET_BUFFER_SIZE) % NSL_PACKET_BUFFER_SIZE) + NSL_SEQ_MODULO) % NSL_SEQ_MODULO;
		}

		void HistoryBuffer::addSeq(double time, ObjectManager* objectManager, std::map<unsigned int, Peer*>* peers)
		{
			if (validUpdatesCounter > 0) {
				int prevIndex = currentIndex;
				currentIndex = (currentIndex + 1) % NSL_PACKET_BUFFER_SIZE_SERVER;
				objectManager->clearBufferIndex(currentIndex, prevIndex);
				for(std::map<unsigned int, Peer*>::iterator it = peers->begin(); it != peers->end(); it++) {
					it->second->clearIndex(currentIndex);
				}
			}
			// if this is first update, keep currentIndex at 0 (and data are already cleared)
			timeData[currentIndex] = time;
			validUpdatesCounter++;
		}

		int HistoryBuffer::getCurrentSeqIndex(void)
		{
			return currentIndex;
		}

		double HistoryBuffer::getTime(int bufferIndex)
		{
			return timeData[bufferIndex];
		}

		bool HistoryBuffer::isSeqInBounds(seqNumber seq)
		{
			if (validUpdatesCounter < NSL_PACKET_BUFFER_SIZE_SERVER) {
				// buffer is not full yet, bounds are limited to validUpdatesCounter
				return seq <= validUpdatesCounter;
			} else {
				// whole buffer is full now, so every seq smaller than currentSeq by less than buffer size is in bounds
				return ((currentSeq - seq + NSL_SEQ_MODULO) % NSL_SEQ_MODULO) < NSL_PACKET_BUFFER_SIZE;
			}
		}

		bool HistoryBuffer::isSecondSeqGreater(seqNumber first, seqNumber second)
		{
			return ((second - first + NSL_SEQ_MODULO - 1) % NSL_SEQ_MODULO) < (NSL_SEQ_MODULO / 2);
		}
	};
};