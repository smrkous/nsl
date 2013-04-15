#include "HistoryBuffer.h"
#include "ObjectManager.h"

#include <algorithm>

namespace nsl {
	namespace client {

		HistoryBuffer::HistoryBuffer(void)
		{
			std::fill( validData, validData + sizeof( validData ), false );
			lastSeq = 0;
			validUpdatesCounter = 0;
			networkIndex = NSL_UNDEFINED_BUFFER_INDEX;
			networkAckIndex = NSL_UNDEFINED_BUFFER_INDEX;
			firstDataIndex = NSL_UNDEFINED_BUFFER_INDEX;
			applicationIndex = NSL_UNDEFINED_BUFFER_INDEX;
			neccessaryIndexBeforeNetworkCount = 0;
		}

		HistoryBuffer::~HistoryBuffer(void)
		{
		}

		bool HistoryBuffer::isEmpty(void)
		{
			return validUpdatesCounter < 1;
		}

		int HistoryBuffer::getLastSeqIndex(void)
		{
			return networkIndex;
		}

		int HistoryBuffer::seqToIndex(seqNumber seq)
		{
			return (networkIndex - ((lastSeq - seq + NSL_SEQ_MODULO) % NSL_SEQ_MODULO) + NSL_PACKET_BUFFER_SIZE) % NSL_PACKET_BUFFER_SIZE;
		}

		seqNumber HistoryBuffer::indexToSeq(int index)
		{
			return (lastSeq - ((networkIndex - index + NSL_PACKET_BUFFER_SIZE) % NSL_PACKET_BUFFER_SIZE) + NSL_SEQ_MODULO) % NSL_SEQ_MODULO;
		}

		bool HistoryBuffer::isSeqAwaited(seqNumber seq)
		{
			if (isEmpty()) {
				return true;
			}

			// count future position of new seq (the distance from last index)
			int distanceFromLastIndex = (seq - lastSeq + NSL_SEQ_MODULO) % NSL_SEQ_MODULO;

			return distanceFromLastIndex > 0 && distanceFromLastIndex < NSL_PACKET_BUFFER_SIZE - neccessaryIndexBeforeNetworkCount;
		}

		bool HistoryBuffer::pushSeq(seqNumber seq, seqNumber ack, double time, ObjectManager* objectManager)
		{
			// is ack missing in buffer?
			if (ack != seq && (!isSeqInBounds(ack) || !isIndexValid(seqToIndex(ack)))) {
				return false;
			}

			// if seq is awaited
			if (isSeqAwaited(seq)) {
				// we search in future, so we cannot use seqToIndex, because it works only for seqs in buffer bounds
				int index = (networkIndex + (seq - lastSeq + NSL_SEQ_MODULO) % NSL_SEQ_MODULO) % NSL_PACKET_BUFFER_SIZE;

				// invalidate all skipped data
				// isSeqAwaited returns false for seq = lastSeq, so index != lastIndex now
				networkIndex = (networkIndex + 1) % NSL_PACKET_BUFFER_SIZE;
				while (networkIndex != index) {
					validData[networkIndex] = false;
					objectManager->clearBufferIndex(networkIndex);
					// invalidate first index, if rewritten
					if (firstDataIndex == networkIndex) {
						firstDataIndex = NSL_UNDEFINED_BUFFER_INDEX;
					}
					networkIndex = (networkIndex + 1) % NSL_PACKET_BUFFER_SIZE;
				}

				// invalidate first index, if rewritten (last index in cycle stayed unchecked)
				if (firstDataIndex == networkIndex) {
					firstDataIndex = NSL_UNDEFINED_BUFFER_INDEX;
				}

				// if first seq is being set, save its index
				if (isEmpty()) {
					firstDataIndex = networkIndex;
				}
				
				// set new values
				validData[networkIndex] = true;
				timeData[networkIndex] = time;
				lastSeq = seq;

				// update ack index
				if (networkAckIndex == NSL_UNDEFINED_BUFFER_INDEX || isSecondSeqGreater(indexToSeq(networkAckIndex), ack)) {
					networkAckIndex = seqToIndex(ack);
				}

				updateNeccessaryIndexCount();
				validUpdatesCounter++;
				return true;

			// if seq was skipped, is there a hole it fills?
			} else if (isSeqInBounds(seq)) {
				int index = seqToIndex(seq);
				if (isIndexValid(index)) {
					return false;
				} else {
					validData[index] = true;
					timeData[index] = time;

					// update ack index
					if (isSecondSeqGreater(indexToSeq(networkAckIndex), ack)) {
						networkAckIndex = seqToIndex(ack);
					}

					updateNeccessaryIndexCount();
					validUpdatesCounter++;
					return true;
				}

			// seq is not relevant
			} else {
				return false;
			}
		}

		bool HistoryBuffer::isSeqInBounds(seqNumber seq)
		{
			if (isEmpty()) {
				return false;
			}
			
			if (firstDataIndex != NSL_UNDEFINED_BUFFER_INDEX) {
				// buffer is not full yet, bounds are limited by first data index from below
				return ((lastSeq - seq + NSL_SEQ_MODULO) % NSL_SEQ_MODULO) <= (networkIndex - firstDataIndex + NSL_PACKET_BUFFER_SIZE) % NSL_PACKET_BUFFER_SIZE;
			} else {
				// whole buffer is full now, so every seq smaller than lastSeq by less than buffer size is in bounds
				return ((lastSeq - seq + NSL_SEQ_MODULO) % NSL_SEQ_MODULO) < NSL_PACKET_BUFFER_SIZE;
			}
		}

		bool HistoryBuffer::isIndexValid(int index)
		{
			return validData[index];
		}

		bool HistoryBuffer::isSecondSeqGreater(seqNumber first, seqNumber second)
		{
			return ((second - first + NSL_SEQ_MODULO - 1) % NSL_SEQ_MODULO) < (NSL_SEQ_MODULO / 2);
		}

		int HistoryBuffer::getFirstSeqIndex() 
		{
			return firstDataIndex;
		}

		int HistoryBuffer::getNextValidIndex(int index)
		{
			// iterate trough all buffer indexes until reaching networkIndex
			// start with next index from the passed one (if networkIndex passed, return immediately)
			while (index != networkIndex) {
				index = (index + 1) % NSL_PACKET_BUFFER_SIZE;
				if (validData[index]) {
					return index;
				}
			}

			// last run of the cycle checked networkIndex (increment is before validity check), so there is nothing left
			return NSL_UNDEFINED_BUFFER_INDEX;
		}

		int HistoryBuffer::getPreviousValidIndex(int index)
		{
			// start with previous index (networkIndex is the last valid index, that this function accepts)
			index = (index - 1 + NSL_PACKET_BUFFER_SIZE) % NSL_PACKET_BUFFER_SIZE;

			// iterate trough all buffer indexes, until reaching networkIndex
			while (index != networkIndex) {
				if (validData[index]) {
					return index;
				}
				index = (index - 1 + NSL_PACKET_BUFFER_SIZE) % NSL_PACKET_BUFFER_SIZE;
			}

			// do not check network index, end with previous index
			return NSL_UNDEFINED_BUFFER_INDEX;
		}

		bool HistoryBuffer::updateApplicationIndex(double time)
		{
			if (isEmpty()) {
				return false;
			}

			// iterate from last acquired data
			int index = networkIndex;
			while (timeData[index] > time) {
				index = getPreviousValidIndex(index);
				if (index == NSL_UNDEFINED_BUFFER_INDEX) {
					return false;
				}
			}

			updateNeccessaryIndexCount();
			applicationIndex = index;
			return true;
		}

		int HistoryBuffer::getApplicationIndex(void)
		{
			return applicationIndex;
		}

		long HistoryBuffer::getValidUpdatesCount(void) 
		{
			return validUpdatesCounter;
		}

		int HistoryBuffer::getFirstNeccessaryIndex(void)
		{
			return (networkIndex - neccessaryIndexBeforeNetworkCount + NSL_PACKET_BUFFER_SIZE) % NSL_PACKET_BUFFER_SIZE;
		}

		void HistoryBuffer::updateNeccessaryIndexCount(void)
		{
			// make sure, that application index and few previous indexes are not rewritten
			// if application index is not set yet, make sure, that firstDataIndex is not rewritten
			int applicationFromNetworkDistance = (applicationIndex == NSL_UNDEFINED_BUFFER_INDEX ?
				(networkIndex - firstDataIndex + NSL_PACKET_BUFFER_SIZE) % NSL_PACKET_BUFFER_SIZE : 
				(networkIndex - applicationIndex + NSL_PACKET_BUFFER_SIZE) % NSL_PACKET_BUFFER_SIZE + NSL_INTERPOLATION_MINIMAL_DATA_COUNT
			);
			// make sure, that last server ack is not rewritten
			int ackFromNetworkDistance = (networkIndex - networkAckIndex + NSL_PACKET_BUFFER_SIZE) % NSL_PACKET_BUFFER_SIZE;

			// from previous restrictions choose the non-rewritable interval size
			neccessaryIndexBeforeNetworkCount = max(applicationFromNetworkDistance, ackFromNetworkDistance);
		}

		double HistoryBuffer::getAverageTimeInterval(unsigned int intervalCount)
		{
			if (validUpdatesCounter < 2 || intervalCount < 1) {
				return 0;
			}

			double totalTime = 0;
			unsigned int totalCount = 0;

			int index = networkIndex;
			double nextTime = timeData[index];
			index = getPreviousValidIndex(index);

			while(totalCount < intervalCount && index != NSL_UNDEFINED_BUFFER_INDEX) {
				totalTime += nextTime - timeData[index];
				nextTime = timeData[index];
				index = getPreviousValidIndex(index);
				totalCount++;
			}

			return totalTime/totalCount;
		}

		double HistoryBuffer::getTime(int index)
		{
			return timeData[index];
		}

		int HistoryBuffer::getLastAckIndex(void)
		{
			return networkAckIndex;
		}

	};
};