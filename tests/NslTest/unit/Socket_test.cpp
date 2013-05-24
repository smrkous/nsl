#include "gtest/gtest.h"
#include "src/Socket.h"
#include <iostream>
#include <string>

#include <bitset>

TEST(Socket_Unit, addressAcceptance) {
	nsl::Socket socket;
	nsl::Address address;
	socket.getAddressFromStrings(address, "127.0.0.1", "30000");
	char* host, *port;
	socket.getStringsFromAddress(address, host, port);
	EXPECT_EQ(0, std::string("127.0.0.1").compare(host));
	EXPECT_EQ(0, std::string("30000").compare(port));
}