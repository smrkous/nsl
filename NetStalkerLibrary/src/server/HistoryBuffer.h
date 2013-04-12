#pragma once

namespace nsl {
	namespace server {
		class ObjectManager;
		class Peer;
	};
};

#include "../common.h"
#include <map>

namespace nsl {
	namespace server {

		/// Manages seqence numbers and maps them to indexes in cyclic history buffer
		/// [firstNeccessaryIndex, lastSeqIndex] is interval of application neccessary data
		class HistoryBuffer 
		{
		private:
			double timeData[NSL_PACKET_BUFFER_SIZE];
			seqNumber currentSeq;		// current seq (stored in currentIndex)
			int currentIndex;			// current index, updates are written in here
			long validUpdatesCounter;
		public:
			HistoryBuffer(void);
			~HistoryBuffer(void);

			/// seq -> buffer index conversion
			/// if seq is not within current buffer bounds, result is undefined
			int seqToIndex(seqNumber); 

			/// buffer index -> seq conversion
			/// if the buffer is not filled yet, future seq is returned
			seqNumber indexToSeq(int);

			/// return index of the current seq
			int getCurrentSeqIndex(void);

			/// Is there index in history buffer for this seq?
			bool isSeqInBounds(seqNumber);

			/// Get time for given index
			/// if the given index is not valid (there are no data in buffer on this index), result is undefined
			double getTime(int);

			/// Add next seq to buffer
			/// Previous values in that buffer index are deleted (using object manager)
			/// Old index will be cleared in object manager and in peers
			void addSeq(double, ObjectManager*, std::map<unsigned int, Peer*>*);
			
			/// seq comparator using modulo
			/// compared by half of seq max value
			bool isSecondSeqGreater(seqNumber first, seqNumber second);
		};

	};
};