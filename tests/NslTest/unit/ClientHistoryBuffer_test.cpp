#include <iostream>

#include "gtest/gtest.h"
#include "src/client/HistoryBuffer.h"

TEST(ClientHistoryBuffer_Unit, emptyBuffer) {
	nsl::client::HistoryBuffer hb;
	EXPECT_EQ(NSL_UNDEFINED_BUFFER_INDEX, hb.getApplicationIndex());
	EXPECT_EQ(NSL_UNDEFINED_BUFFER_INDEX, hb.getFirstNeccessaryIndex());
	EXPECT_EQ(NSL_UNDEFINED_BUFFER_INDEX, hb.getFirstSeqIndex());
	EXPECT_EQ(NSL_UNDEFINED_BUFFER_INDEX, hb.getLastAckIndex());
	EXPECT_EQ(NSL_UNDEFINED_BUFFER_INDEX, hb.getLastSeqIndex());
	EXPECT_EQ(0, hb.getValidUpdatesCount());
	EXPECT_EQ(true, hb.isEmpty());
	for(unsigned int a = 0; a < NSL_SEQ_MODULO; a++) {
		EXPECT_EQ(false, hb.isSeqInBounds(a));
		EXPECT_EQ(true, hb.isSeqAwaited(a));
	}
}

TEST(ClientHistoryBuffer_Unit, acceptanceOfUpdates) {
	nsl::client::HistoryBuffer hb;
	int firstIndexToClear;
	int lastIndexToClear;

	EXPECT_EQ(false, hb.pushSeq(0, 12, 23, firstIndexToClear, lastIndexToClear, 1.0));
	EXPECT_EQ(false, hb.pushSeq(2, 1, 23, firstIndexToClear, lastIndexToClear, 1.0));

	EXPECT_EQ(true, hb.pushSeq(10, 10, 23, firstIndexToClear, lastIndexToClear, 1.0));
	EXPECT_EQ(false, hb.pushSeq(10, 10, 23, firstIndexToClear, lastIndexToClear, 1.0));
	EXPECT_EQ(true, hb.pushSeq(13, 13, 23, firstIndexToClear, lastIndexToClear, 1.0));
	EXPECT_EQ(true, hb.pushSeq(14, 13, 23, firstIndexToClear, lastIndexToClear, 1.0));
	EXPECT_EQ(true, hb.pushSeq(11, 11, 23, firstIndexToClear, lastIndexToClear, 1.0));
	EXPECT_EQ(true, hb.pushSeq(12, 10, 23, firstIndexToClear, lastIndexToClear, 1.0));
	EXPECT_EQ(false, hb.pushSeq(15, 9, 23, firstIndexToClear, lastIndexToClear, 1.0));
	EXPECT_EQ(false, hb.pushSeq(NSL_PACKET_BUFFER_SIZE + 15, 10, 23, firstIndexToClear, lastIndexToClear, 1.0));
}

TEST(ClientHistoryBuffer_Unit, ackIndex) {
	nsl::client::HistoryBuffer hb;
	int firstIndexToClear;
	int lastIndexToClear;

	EXPECT_EQ(true, hb.pushSeq(10, 10, 23, firstIndexToClear, lastIndexToClear, 1.0));
	EXPECT_EQ(9, hb.getLastAckIndex());
	EXPECT_EQ(true, hb.pushSeq(12, 12, 23, firstIndexToClear, lastIndexToClear, 1.0));
	EXPECT_EQ(9, hb.getLastAckIndex());
	EXPECT_EQ(true, hb.pushSeq(11, 11, 23, firstIndexToClear, lastIndexToClear, 1.0));
	EXPECT_EQ(9, hb.getLastAckIndex());
	EXPECT_EQ(true, hb.pushSeq(13, 10, 23, firstIndexToClear, lastIndexToClear, 1.0));
	EXPECT_EQ(9, hb.getLastAckIndex());
	EXPECT_EQ(true, hb.pushSeq(14, 11, 23, firstIndexToClear, lastIndexToClear, 1.0));
	EXPECT_EQ(10, hb.getLastAckIndex());
}


TEST(ClientHistoryBuffer_Unit, indexClears) {
	nsl::client::HistoryBuffer hb;
	int firstIndexToClear;
	int lastIndexToClear;

	EXPECT_EQ(true, hb.pushSeq(10, 10, 23, firstIndexToClear, lastIndexToClear, 1.0));
	EXPECT_EQ(0, firstIndexToClear);
	EXPECT_EQ(9, lastIndexToClear);

	EXPECT_EQ(true, hb.pushSeq(12, 12, 23, firstIndexToClear, lastIndexToClear, 1.0));
	EXPECT_EQ(10, firstIndexToClear);
	EXPECT_EQ(11, lastIndexToClear);

	EXPECT_EQ(true, hb.pushSeq(11, 11, 23, firstIndexToClear, lastIndexToClear, 1.0));
	EXPECT_EQ(NSL_UNDEFINED_BUFFER_INDEX, firstIndexToClear);
	EXPECT_EQ(NSL_UNDEFINED_BUFFER_INDEX, lastIndexToClear);
}


TEST(ClientHistoryBuffer_Unit, validIndexIteration) {
	nsl::client::HistoryBuffer hb;
	int firstIndexToClear;
	int lastIndexToClear;

	EXPECT_EQ(true, hb.pushSeq(NSL_PACKET_BUFFER_SIZE - 4, NSL_PACKET_BUFFER_SIZE - 4, 23, firstIndexToClear, lastIndexToClear, 1.0));
	EXPECT_EQ(true, hb.pushSeq(NSL_PACKET_BUFFER_SIZE - 2, NSL_PACKET_BUFFER_SIZE - 2, 23, firstIndexToClear, lastIndexToClear, 1.0));
	EXPECT_EQ(true, hb.pushSeq(NSL_PACKET_BUFFER_SIZE - 1, NSL_PACKET_BUFFER_SIZE - 1, 23, firstIndexToClear, lastIndexToClear, 1.0));
	EXPECT_EQ(true, hb.pushSeq(NSL_PACKET_BUFFER_SIZE + 2, NSL_PACKET_BUFFER_SIZE + 2, 23, firstIndexToClear, lastIndexToClear, 1.0));
	EXPECT_EQ(true, hb.pushSeq(NSL_PACKET_BUFFER_SIZE + 5, NSL_PACKET_BUFFER_SIZE + 5, 23, firstIndexToClear, lastIndexToClear, 1.0));

	int lastSeqIndex = hb.getLastSeqIndex();
	EXPECT_EQ(NSL_UNDEFINED_BUFFER_INDEX, hb.getNextValidIndex(lastSeqIndex));
	int index = hb.getPreviousValidIndex(lastSeqIndex);
	int expected = (lastSeqIndex - 3 + NSL_PACKET_BUFFER_SIZE) % NSL_PACKET_BUFFER_SIZE;
	EXPECT_EQ(expected, index);
	index = hb.getPreviousValidIndex(index);
	expected = (lastSeqIndex - 6 + NSL_PACKET_BUFFER_SIZE) % NSL_PACKET_BUFFER_SIZE;
	EXPECT_EQ(expected, index);
	index = hb.getPreviousValidIndex(index);
	expected = (lastSeqIndex - 7 + NSL_PACKET_BUFFER_SIZE) % NSL_PACKET_BUFFER_SIZE;
	EXPECT_EQ(expected, index);
	index = hb.getPreviousValidIndex(index);
	expected = (lastSeqIndex - 9 + NSL_PACKET_BUFFER_SIZE) % NSL_PACKET_BUFFER_SIZE;
	EXPECT_EQ(expected, index);
	EXPECT_EQ(NSL_UNDEFINED_BUFFER_INDEX, hb.getPreviousValidIndex(index));

	index = hb.getNextValidIndex(index);
	expected = (lastSeqIndex - 7 + NSL_PACKET_BUFFER_SIZE) % NSL_PACKET_BUFFER_SIZE;
	EXPECT_EQ(expected, index);
	index = hb.getNextValidIndex(index);
	expected = (lastSeqIndex - 6 + NSL_PACKET_BUFFER_SIZE) % NSL_PACKET_BUFFER_SIZE;
	EXPECT_EQ(expected, index);
	index = hb.getNextValidIndex(index);
	expected = (lastSeqIndex - 3 + NSL_PACKET_BUFFER_SIZE) % NSL_PACKET_BUFFER_SIZE;
	EXPECT_EQ(expected, index);
	index = hb.getNextValidIndex(index);
	EXPECT_EQ(lastSeqIndex, index);
}

TEST(ClientHistoryBuffer_Unit, averageTickDuration) {
	nsl::client::HistoryBuffer hb;
	int firstIndexToClear;
	int lastIndexToClear;

	EXPECT_EQ(true, hb.pushSeq(10, 10, 1, firstIndexToClear, lastIndexToClear, 1.0));
	EXPECT_EQ(true, hb.pushSeq(12, 12, 3, firstIndexToClear, lastIndexToClear, 1.0));
	EXPECT_EQ(2, hb.getAverageTimeInterval(1));
	EXPECT_EQ(true, hb.pushSeq(11, 11, 2, firstIndexToClear, lastIndexToClear, 1.0));
	EXPECT_EQ(1, hb.getAverageTimeInterval(2));
	EXPECT_EQ(true, hb.pushSeq(13, 13, 5, firstIndexToClear, lastIndexToClear, 1.0));
	EXPECT_EQ(true, hb.pushSeq(14, 14, 10, firstIndexToClear, lastIndexToClear, 1.0));
	EXPECT_EQ(5, hb.getAverageTimeInterval(1));
	EXPECT_EQ(2.25, hb.getAverageTimeInterval(4));

}
