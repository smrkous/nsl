#pragma once

#include "../configuration.h"
#include <vector>
#include <map>
#include <iterator>

namespace nsl {
	namespace client {
		
		/// First seq sent is 1, server ack is 0 (but there are no data)
		class CustomMessageBuffer 
		{
		private:
			std::vector<std::pair<byte*, unsigned int> > bufferedMessages[NSL_CUSTOM_MESSAGE_BUFFER_SIZE];
			int currentIndex;
			seqNumber currentSeq;
			int ackIndex;
		public:
			CustomMessageBuffer(void);
			~CustomMessageBuffer(void);
			
			/// seq -> buffer index conversion
			/// if seq is not within current buffer bounds, result is undefined
			int seqToIndex(seqNumber); 

			/// buffer index -> seq conversion
			/// if the buffer is not filled yet, future seq is returned
			seqNumber indexToSeq(int);

			/// return index of the current seq
			int getCurrentSeqIndex(void);

			/// Add next seq to buffer
			/// Previous values in that buffer index are deleted
			void addSeq(void);

			/// wipe all data from that index (scheduled actions, object data, etc.) so it can be used to store new data
			void updateAck(seqNumber ack);

			/// Get first index in buffer, which was not acked by server
			int getFirstUnackedIndex(void);

			/// Get next valid index.
			int getNextValidIndex(int index);

			/// seq comparator using modulo
			/// compared by half of seq max value
			bool isSecondSeqGreater(seqNumber first, seqNumber second);

			const std::vector<std::pair<byte*, unsigned int> >::iterator bufferedMessagesBegin(int bufferIndex) {return bufferedMessages[bufferIndex].begin();}
			const std::vector<std::pair<byte*, unsigned int> >::iterator bufferedMessagesEnd(int bufferIndex) {return bufferedMessages[bufferIndex].end();}
			bool isIndexEmpty(int bufferIndex) {return bufferedMessages[bufferIndex].empty();}
			void addBufferedMessage(byte* data, unsigned int size) {return bufferedMessages[currentIndex].push_back(std::pair<byte*, unsigned int>(data, size));}
		};
	};
};
