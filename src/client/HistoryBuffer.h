#pragma once
#include "../common.h"
#include "ObjectManager.h"

namespace nsl {
	namespace client {

		/// Manages seqence numbers and maps them to indexes in history buffer
		class HistoryBuffer 
		{
		private:
			bool validData[PACKET_BUFFER_SIZE];
			double timeData[PACKET_BUFFER_SIZE];
			seqType lastSeq;		// last received seq (stored in networkIndex)
			int networkIndex;		// last index written by network
			int firstDataIndex;		// index of first received seq - it is available only between first packet arrival and rewriting due to cyclic buffer
			int applicationIndex;	// last index, which application already passed in time (never exceeds networkIndex)
		public:
			HistoryBuffer(void);
			~HistoryBuffer(void);

			/// has no seq been received yet?
			bool isEmpty(void);

			/// seq -> buffer index conversion
			/// if seq is not within current buffer bounds, result is undefined
			/// note, that even when in bounds, this seq could have been skipped (not received by client) - check this by calling isSeqPresent(index)
			int seqToIndex(seqType); 

			/// buffer index -> seq conversion
			/// note, that returned seq could have been skipped (not received by client) - check this by calling isSeqPresent(index)
			seqType indexToSeq(int);

			/// return index of the newest present seq
			/// if isEmpty() returns true, UNDEFINED_BUFFER_INDEX is returned
			int getLastSeqIndex(void);

			/// Is there index in history buffer for this seq?
			/// if the buffer is empty, it always returns false (bounds are not defined yet)
			bool isSeqInBounds(seqType);

			/// is this seq not accepted yet and awaited (from close future)?
			/// if the buffer is empty, it always returns true (every seq is awaited)
			bool isSeqAwaited(seqType);

			/// was this seq received or skipped? (by index)
			bool isSeqPresent(int);

			/// get time for which were the data in seq snapshoted
			/// if isSeqPresent(int) returns false for this index, result is undefined
			double getTime(int);

			/// push incoming seq into buffer 
			/// returns false, if seq was not pushed due to a failure - (seq must be awaited or in bounds and not already present)
			/// if new seq was pushed, object manager is told to clear all skipped data
			bool pushSeq(seqType, double, ObjectManager*);

			/// finds next index, which seq data are present in buffer
			/// if last valid index is passed to this function, UNDEFINED_BUFFER_INDEX is returned
			int getNextValidIndex(int);

			/// finds previous index, which seq data are present in buffer
			/// if first valid index is passed to this function, UNDEFINED_BUFFER_INDEX is returned
			int getPreviousValidIndex(int);

			/// seq comparator using modulo
			/// compared by half of seq max value
			bool isSecondSeqGreater(seqType first, seqType second);

			/// get index of first received data
			/// return UNDEFINED_BUFFER_INDEX, if no data arrived yet or those data were already overriden
			int getFirstSeqIndex(void);

			/// get the last index, that passes over given time
			/// if the given time exceeds buffer, last valid index is returned
			/// if there are no data in buffer, or no data with lower time that specified, UNDEFINED_BUFFER_INDEX is returned
			/// this also sets that index (if successfuly found) as application current position and sets accordingly the bounds of indexes which can be overriden by new data 
			int updateApplicationIndex(double time);
		};

	};
};