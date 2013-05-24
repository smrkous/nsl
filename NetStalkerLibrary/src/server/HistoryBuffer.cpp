/*
 * Copyright (C) 2013 Petr Smrcek
 * This file is originally a part of the Net Stalker Library
 * For conditions of distribution and use, see copyright notice in nsl.h
 */

#include "HistoryBuffer.h"
#include "ObjectManager.h"
#include "Peer.h"

#include <algorithm>

namespace nsl {
	namespace server {

		HistoryBuffer::HistoryBuffer(void)
		{
			currentSeq = 0;
			currentIndex = NSL_UNDEFINED_BUFFER_INDEX;
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
				currentSeq = (currentSeq + 1) % NSL_SEQ_MODULO;
				currentIndex = (currentIndex + 1) % NSL_PACKET_BUFFER_SIZE_SERVER;
				objectManager->clearBufferIndex(currentIndex, prevIndex);
				for(std::map<unsigned int, Peer*>::iterator it = peers->begin(); it != peers->end(); it++) {
					it->second->clearIndex(currentIndex);
				}
			} else {
				// if this is first update, set currentIndex to 0 (data are already cleared)
				currentIndex = 0;
			}
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