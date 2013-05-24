#include <iostream>

#include "gtest/gtest.h"
#include "include/nslBitStream.h"

TEST(BitStreamWriter_Unit, expandingSize) {
	nsl::byte data[20] = {};
	
	nsl::BitStreamWriter writer(2);
	EXPECT_EQ(0, writer.getBitSize());

	writer.write(2, data);
	EXPECT_EQ(2*8, writer.getBitSize());
	
	writer.write(2, data);
	EXPECT_LT(2*8, writer.getBitSize());
	
	writer.write(20, data);
	EXPECT_LE(24*8, writer.getBitSize());
}

TEST(BitStreamWriter_Unit, externalBuffer) {
	nsl::byte data[20] = {};
	nsl::BitStreamWriter writer(data, 20);

	writer.write(20, data);

	EXPECT_ANY_THROW(writer.write(1, data));

	EXPECT_NO_THROW(data[2]);
}

TEST(BitStreamWriter_Unit, writeValue) {
	nsl::byte data[2] = {1,2};
	nsl::byte writeBuffer[2] = {};
	nsl::BitStreamWriter writer(writeBuffer, 2);

	writer.write(2, data);

	if (nsl::isLittleEndian()) {
		EXPECT_EQ(1, data[0]);
		EXPECT_EQ(2, data[1]);
	} else {
		EXPECT_EQ(2, data[0]);
		EXPECT_EQ(1, data[1]);
	}	
}
