#include "HistoryBuffer.h"

#include <algorithm>

namespace nsl {
	namespace client {

		HistoryBuffer::HistoryBuffer(void)
		{
			std::fill( validData, validData + sizeof( validData ), false );
			lastSeq = 0;
			networkIndex = UNDEFINED_BUFFER_INDEX;
			firstDataIndex = UNDEFINED_BUFFER_INDEX;
			applicationIndex = UNDEFINED_BUFFER_INDEX;
		}

		HistoryBuffer::~HistoryBuffer(void)
		{
		}

		bool HistoryBuffer::isEmpty(void)
		{
			return networkIndex == UNDEFINED_BUFFER_INDEX;
		}

		int HistoryBuffer::getLastSeqIndex(void)
		{
			return networkIndex;
		}

		int HistoryBuffer::seqToIndex(seqType seq)
		{
			return (networkIndex - ((lastSeq - seq + SEQ_MODULO) % SEQ_MODULO) + PACKET_BUFFER_SIZE) % PACKET_BUFFER_SIZE;
		}

		seqType HistoryBuffer::indexToSeq(int index)
		{
			return (lastSeq - ((networkIndex - index + PACKET_BUFFER_SIZE)%PACKET_BUFFER_SIZE) + SEQ_MODULO)%SEQ_MODULO;
		}

		bool HistoryBuffer::isSeqAwaited(seqType seq)
		{
			// data NSL_INTERPOLATION_MINIMAL_DATA_COUNT indexes behind application index are forbidden to be rewritten
			// +1 and < (instead of <=, because right side is max allowed distance) are there to exclude seq = lastSeq
			return isEmpty() || applicationIndex == UNDEFINED_BUFFER_INDEX ||
				(seq - lastSeq + 1 + SEQ_MODULO) % SEQ_MODULO < (applicationIndex - networkIndex - NSL_INTERPOLATION_MINIMAL_DATA_COUNT);
		}

		bool HistoryBuffer::pushSeq(seqType seq, double time, ObjectManager* objectManager)
		{
			// if seq is awaited
			if (isSeqAwaited(seq)) {
				// we search in future, so we cannot use seqToIndex, because it works only for seqs in buffer bounds
				int index = (networkIndex + (seq - lastSeq + SEQ_MODULO)%SEQ_MODULO) % PACKET_BUFFER_SIZE;

				// invalidate all skipped data
				// isSeqAwaited returns false for seq = lastSeq, so index != lastIndex now
				networkIndex = (networkIndex + 1) % PACKET_BUFFER_SIZE;
				while (networkIndex != index) {
					validData[networkIndex] = false;
					objectManager->clearBufferIndex(networkIndex);
					// invalidate first index, if rewritten
					if (firstDataIndex == networkIndex) {
						firstDataIndex = UNDEFINED_BUFFER_INDEX;
					}
					networkIndex = (networkIndex + 1) % PACKET_BUFFER_SIZE;
				}

				// invalidate first index, if rewritten (last index in cycle stayed unchecked)
				if (firstDataIndex == networkIndex) {
					firstDataIndex = UNDEFINED_BUFFER_INDEX;
				}

				// if first seq is being set, save its index
				if (isEmpty()) {
					firstDataIndex = networkIndex;
				}
				
				// set new values
				validData[networkIndex] = true;
				timeData[networkIndex] = time;
				lastSeq = seq;

				return true;

			// if seq was skipped
			} else if (isSeqInBounds(seq)) {
				int index = seqToIndex(seq);
				if (isSeqPresent(index)) {
					return false;
				} else {
					validData[index] = true;
					timeData[index] = time;
					return true;
				}

			// seq is not relevant
			} else {
				return false;
			}
		}

		bool HistoryBuffer::isSeqInBounds(seqType seq)
		{
			if (isEmpty()) {
				return false;
			}
			
			return ((lastSeq - seq + SEQ_MODULO) % SEQ_MODULO) < PACKET_BUFFER_SIZE;
		}

		bool HistoryBuffer::isSeqPresent(int index)
		{
			return validData[index];
		}

		bool HistoryBuffer::isSecondSeqGreater(seqType first, seqType second)
		{
			return ((second - first + SEQ_MODULO - 1) % SEQ_MODULO) < (SEQ_MODULO / 2);
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
				index = (index++) % PACKET_BUFFER_SIZE;
				if (validData[index]) {
					return index;
				}
			}

			// last run of the cycle checked networkIndex (increment is before validity check), so there is nothing left
			return UNDEFINED_BUFFER_INDEX;
		}

		int HistoryBuffer::getPreviousValidIndex(int index)
		{
			// start with previous index (networkIndex is the last valid index, that this function accepts)
			index = (index--) % PACKET_BUFFER_SIZE;

			// iterate trough all buffer indexes, until reaching networkIndex
			while (index != networkIndex) {
				if (validData[index]) {
					return index;
				}
				index = (index--) % PACKET_BUFFER_SIZE;
			}

			// do not check network index, end with previous index
			return UNDEFINED_BUFFER_INDEX;
		}

		int HistoryBuffer::updateApplicationIndex(double time)
		{
			if (isEmpty()) {
				return UNDEFINED_BUFFER_INDEX;
			}

			// iterate from last acquired data
			int index = networkIndex;
			while (timeData[index] > time) {
				index = getPreviousValidIndex(index);
				if (index == UNDEFINED_BUFFER_INDEX) {
					return UNDEFINED_BUFFER_INDEX;
				}
			}

			applicationIndex = index;
			return index;
		}
	};
};