/*
 * Copyright (C) 2013 Petr Smrcek
 * This file is originally a part of the Net Stalker Library
 * For conditions of distribution and use, see copyright notice in nsl.h
 */

#pragma once

namespace nsl {
	namespace client {
		class ObjectManager;
	};
};

#include "../configuration.h"

namespace nsl {
	namespace client {

		/// Manages seqence numbers and maps them to indexes in cyclic history buffer
		/// [firstNeccessaryIndex, lastSeqIndex] is interval of application neccessary data
		class HistoryBuffer 
		{
		private:
			bool validData[NSL_PACKET_BUFFER_SIZE];
			double timeData[NSL_PACKET_BUFFER_SIZE];
			seqNumber lastSeq;		// last received seq (stored in networkIndex)
			int networkIndex;		// last index written by network
			int networkAckIndex;	// last ack the server knows about
			int firstDataIndex;		// index of first received seq - it is available only between first packet arrival and rewriting due to cyclic buffer
			int applicationIndex;	// last index, which application already passed in time (never exceeds networkIndex)
			int neccessaryIndexBeforeNetworkCount; // number of neccessary (cannot be deleted) indexes before networkIndex (which is neccessary always)
			long validUpdatesCounter;
			double lastUpdateApplicationTime;

			void updateNeccessaryIndexCount(void);
		public:
			HistoryBuffer(void);
			~HistoryBuffer(void);

			/// has no seq been received yet?
			bool isEmpty(void);

			/// seq -> buffer index conversion
			/// if seq is not within current buffer bounds, result is undefined
			/// note, that even when in bounds, this seq could have been skipped (not received by client) - check this by calling isSeqPresent(index)
			int seqToIndex(seqNumber); 

			/// buffer index -> seq conversion
			/// note, that returned seq could have been skipped (not received by client) - check this by calling isSeqPresent(index)
			seqNumber indexToSeq(int);

			/// return index of the newest present seq
			/// if isEmpty() returns true, UNDEFINED_BUFFER_INDEX is returned
			int getLastSeqIndex(void);

			/// Is there index in history buffer for this seq?
			/// if the buffer is empty, it always returns false (bounds are not defined yet)
			bool isSeqInBounds(seqNumber);

			/// Is this seq awaited (from close future)?
			/// if the buffer is empty, it always returns true (every seq is awaited)
			/// if application index was not set yet and buffer is full, no seq is awaited (first data cannot be rewritten)
			bool isSeqAwaited(seqNumber);

			/// are there valid data on this index?
			bool isIndexValid(int);

			/// Get time for which were the data in seq snapshoted
			/// if the given index is not valid (there are no data in buffer on this index), result is undefined
			double getTime(int);

			/// Push incoming seq into buffer (index is given to this seq).
			/// For successful push (return true) it is required:
			/// 1) seq must be awaited or must fill skipped hole after skipped seq
			/// 2) ack must be present in buffer or eqal to seq
			/// If new seq was pushed, firstIndexToClear and lastIndexToClear define interval of invalidated indexes
			bool pushSeq(seqNumber seq, seqNumber ack, double serverTime, int& firstIndexToClear, int& lastIndexToClear, double applicationTime);

			/// finds next index, which seq data are present in buffer
			/// if last valid index is passed to this function, NSL_UNDEFINED_BUFFER_INDEX is returned
			int getNextValidIndex(int);

			/// finds previous index, which seq data are present in buffer
			/// if first valid index is passed to this function, NSL_UNDEFINED_BUFFER_INDEX is returned
			int getPreviousValidIndex(int);

			/// finds next index, 
			/// 0 <= given index < NSL_PACKET_BUFFER_SIZE - otherwise result is NSL_UNDEFINED_BUFFER_INDEX
			int getNextIndex(int);

			/// finds previous index, 
			/// 0 <= given index < NSL_PACKET_BUFFER_SIZE - otherwise result is NSL_UNDEFINED_BUFFER_INDEX
			int getPreviousIndex(int);

			/// seq comparator using modulo
			/// compared by half of seq max value
			bool isSecondSeqGreater(seqNumber first, seqNumber second);

			/// get index of first received data
			/// return NSL_UNDEFINED_BUFFER_INDEX, if no data arrived yet or those data were already rewritten
			int getFirstSeqIndex(void);

			/// set the application position to index, which is the last that the application stepped over
			/// (if the given time exceeds buffer, last valid index is returned)
			/// if there are no data in buffer, or no data with lower time that specified, false is returned
			/// this is important to keep up to date, because the buffer will ensure, that the still useful snapshots are not rewritten
			bool updateApplicationIndex(double time);

			/// get the index, which is the last that the application stepped over
			/// if the application index was not successfuly updated at least once, NSL_UNDEFINED_BUFFER_INDEX is returend
			int getApplicationIndex(void);

			/// How many valid seqs was accepted from the first update
			long getValidUpdatesCount(void);

			/// Get first index, which cannot be deleted because there are still some relevant data
			/// if the buffer is empty, result is undefined
			int getFirstNeccessaryIndex(void);

			/// get average interval between neighbour times
			/// count last intervals will be checked - if not enough updates available, all available will be checked
			/// if less then 2 snapshots are in buffer or intervalCount < 2, 0 is returned
			double getAverageTimeInterval(unsigned int intervalCount);

			/// Get index of last server ack
			/// If history buffer is empty, NSL_UNDEFINED_BUFFER_INDEX is returned
			int getLastAckIndex(void);

			/// Get application time when last server packet was received
			double getLastUpdateApplicationTime(void);
		};

	};
};