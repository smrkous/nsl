#include <iostream>

#include "gtest/gtest.h"
#include "include/nslBitStream.h"

TEST(BitStreamReader_Unit, remainingSize) {
	nsl::byte data[10] = {};
	nsl::byte readBuffer[10] = {};
	nsl::BitStreamReader reader(data, 10);
	EXPECT_EQ(10, reader.getRemainingByteSize());
	
	reader.read(2, readBuffer);
	EXPECT_EQ(8, reader.getRemainingByteSize());

	reader.read(8, readBuffer);
	EXPECT_EQ(0, reader.getRemainingByteSize());
}

TEST(BitStreamReader_Unit, readValue) {
	nsl::byte data[2] = {1,2};
	nsl::byte readBuffer[2] = {};
	nsl::BitStreamReader reader(data, 2);

	reader.read(2, readBuffer);

	if (nsl::isLittleEndian()) {
		EXPECT_EQ(1, readBuffer[0]);
		EXPECT_EQ(2, readBuffer[1]);
	} else {
		EXPECT_EQ(2, readBuffer[0]);
		EXPECT_EQ(1, readBuffer[1]);
	}	
}

TEST(BitStreamReader_Unit, skipBits) {
	nsl::byte data[4] = {1,2,3,4};
	nsl::byte readBuffer[4] = {};
	nsl::BitStreamReader reader(data, 4);

	reader.skipBits(16);
	EXPECT_EQ(2, reader.getRemainingByteSize());

	reader.read(1, readBuffer);
	EXPECT_EQ(3, readBuffer[0]);
}

TEST(BitStreamReader_Unit, subreader) {
	nsl::byte data[4] = {1,2,3,4};
	nsl::byte readBuffer[4] = {};
	nsl::BitStreamReader reader(data, 4, false);

	reader.skipBits(8);


	nsl::BitStreamReader* subreader = reader.createSubreader(2, false);

	EXPECT_EQ(3, reader.getRemainingByteSize());
	EXPECT_EQ(2, subreader->getRemainingByteSize());

	subreader->read(1, readBuffer);
	EXPECT_EQ(2, readBuffer[0]);

	subreader->destroy();


	subreader = reader.createSubreader(2, true);
	data[1] = 10;
	reader.read(1, readBuffer);
	EXPECT_EQ(10, readBuffer[0]);
	subreader->read(1, readBuffer);
	EXPECT_EQ(2, readBuffer[0]);

	subreader->destroy();
}

TEST(BitStreamReader_Unit, resetStream) {
	nsl::byte data[4] = {1,2,3,4};
	nsl::byte readBuffer[4] = {};
	nsl::BitStreamReader reader(data, 4);

	reader.skipBits(8);

	EXPECT_EQ(3, reader.getRemainingByteSize());
	
	reader.resetStream();

	EXPECT_EQ(4, reader.getRemainingByteSize());

	reader.resetStream(2);
	EXPECT_EQ(2, reader.getRemainingByteSize());
}