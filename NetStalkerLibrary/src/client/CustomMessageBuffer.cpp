#include "CustomMessageBuffer.h"

namespace nsl {
	namespace client {

		CustomMessageBuffer::CustomMessageBuffer()
		{
			currentIndex = NSL_UNDEFINED_BUFFER_INDEX;
			ackIndex = NSL_UNDEFINED_BUFFER_INDEX;
		}

		CustomMessageBuffer::~CustomMessageBuffer()
		{

		}

		int CustomMessageBuffer::seqToIndex(seqNumber seq)
		{
			return (currentIndex - ((currentSeq - seq + NSL_SEQ_MODULO) % NSL_SEQ_MODULO) + NSL_CUSTOM_MESSAGE_BUFFER_SIZE) % NSL_CUSTOM_MESSAGE_BUFFER_SIZE;
		}

		seqNumber CustomMessageBuffer::indexToSeq(int index)
		{
			return (currentSeq - ((currentIndex - index + NSL_CUSTOM_MESSAGE_BUFFER_SIZE) % NSL_CUSTOM_MESSAGE_BUFFER_SIZE) + NSL_SEQ_MODULO) % NSL_SEQ_MODULO;
		}

		int CustomMessageBuffer::getCurrentSeqIndex(void)
		{
			return currentIndex;
		}

		void CustomMessageBuffer::addSeq(void)
		{
			if (currentIndex == NSL_UNDEFINED_BUFFER_INDEX) {
				currentIndex = 1;
				currentSeq = 1;
				ackIndex = 0;
				return;
			}

			int targetIndex = (currentIndex + 1) % NSL_CUSTOM_MESSAGE_BUFFER_SIZE;
			if (targetIndex == ackIndex) {
				throw Exception(NSL_EXCEPTION_DISCONNECTED, "NSL: client buffer overflowed because server did not send any acks");
			}

			currentIndex = targetIndex;
			currentSeq = (currentSeq + 1) % NSL_SEQ_MODULO;
			if (!isIndexEmpty(currentIndex)) {
				for (std::vector<std::pair<byte*, unsigned int> >::iterator it = bufferedMessagesBegin(currentIndex); it != bufferedMessagesEnd(currentIndex); it++) {
					delete[] it->first;
				}
				bufferedMessages[currentIndex].clear();
			}
		}

		void CustomMessageBuffer::updateAck(seqNumber ack)
		{
			/// update it only if buffer is not empty and new ack is higher
			if (currentIndex != NSL_UNDEFINED_BUFFER_INDEX && isSecondSeqGreater(seqToIndex(ackIndex), ack)) {
				ackIndex = seqToIndex(ack);
			}
		}

		int CustomMessageBuffer::getFirstUnackedIndex(void)
		{
			if (ackIndex == NSL_UNDEFINED_BUFFER_INDEX) {
				return NSL_UNDEFINED_BUFFER_INDEX;
			} else {
				return (ackIndex + 1) % NSL_CUSTOM_MESSAGE_BUFFER_SIZE;
			}
		}

		int CustomMessageBuffer::getNextValidIndex(int index)
		{
			if (index != currentIndex) {
				return (index + 1) % NSL_CUSTOM_MESSAGE_BUFFER_SIZE;
			}
		}

		bool CustomMessageBuffer::isSecondSeqGreater(seqNumber first, seqNumber second)
		{
			return ((second - first + NSL_SEQ_MODULO - 1) % NSL_SEQ_MODULO) < (NSL_SEQ_MODULO / 2);
		}
	};
};