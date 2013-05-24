#include "gtest/gtest.h"
#include "src/configuration.h"
#include "src/client/ProtocolParser.h"
#include "src/client/HistoryBuffer.h"
#include "src/client/ObjectManager.h"
#include "src/client/CustomMessageBuffer.h"
#include "src/ObjectClassDefinition.h"
#include <iostream>
#include <string>

TEST(ClientProtocolParser_Unit, parseHeader) {
	
	nsl::client::HistoryBuffer historyBuffer;
	nsl::client::ObjectManager objectManager(&historyBuffer);
	nsl::client::CustomMessageBuffer customMessageBuffer;
	customMessageBuffer.addSeq();
	nsl::client::ProtocolParser parser(&historyBuffer, &objectManager, &customMessageBuffer);

	nsl::BitStreamWriter writer;
	writer.write<nsl::Attribute<nsl::seqNumber> >(2);
	writer.write<nsl::Attribute<nsl::seqNumber> >(2);
	writer.write<nsl::double64>(25.0);
	writer.write<nsl::Attribute<nsl::seqNumber> >(customMessageBuffer.getCurrentSeqIndex());
	nsl::ObjectFlags flags;
	flags.action = NSL_OBJECT_FLAG_ACTION_END_OF_SECTION;
	writer << flags;
	customMessageBuffer.addSeq();

	unsigned int byteSize;
	nsl::byte* data = writer.toBytes(byteSize);
	nsl::BitStreamReader reader(data, byteSize);

	parser.proccessUpdatePacket(&reader, 20.0);

	unsigned int lastSeqIndex = historyBuffer.getLastSeqIndex();
	EXPECT_EQ(1, lastSeqIndex);
	EXPECT_EQ(25.0, historyBuffer.getTime(lastSeqIndex));
	EXPECT_EQ(customMessageBuffer.getCurrentSeqIndex(), customMessageBuffer.getFirstUnackedIndex());
}

TEST(ClientProtocolParser_Unit, parseNewObjects) {
	
	nsl::client::HistoryBuffer historyBuffer;
	nsl::client::ObjectManager objectManager(&historyBuffer);
	nsl::ObjectClass oc(0);
	oc.defineAttribute<nsl::uint8>(0);
	oc.defineAttribute<nsl::uint32>(1);
	nsl::ObjectClassDefinition ocd(oc);
	objectManager.registerObjectClass(&ocd);
	nsl::client::CustomMessageBuffer customMessageBuffer;
	customMessageBuffer.addSeq();
	nsl::client::ProtocolParser parser(&historyBuffer, &objectManager, &customMessageBuffer);

	nsl::BitStreamWriter writer;
	writer.write<nsl::Attribute<nsl::seqNumber> >(2);
	writer.write<nsl::Attribute<nsl::seqNumber> >(2);
	writer.write<nsl::double64>(25.0);
	writer.write<nsl::Attribute<nsl::seqNumber> >(customMessageBuffer.getCurrentSeqIndex());

	nsl::ObjectFlags flags;
	flags.action = NSL_OBJECT_FLAG_ACTION_CREATE;
	flags.creationCustomMessage = NSL_OBJECT_FLAG_CM_EMPTY;
	for(unsigned int i = 1; i <= 2; i++) {
		writer << flags;
		writer.write<nsl::uint16>(0);
		writer.write<nsl::uint32>(i);
		writer.write<nsl::uint8>(35);
		writer.write<nsl::uint32>(110);
	}
	nsl::ObjectFlags flags2;
	flags2.action = NSL_OBJECT_FLAG_ACTION_END_OF_SECTION;
	writer << flags2;

	unsigned int byteSize;
	nsl::byte* data = writer.toBytes(byteSize);
	nsl::BitStreamReader reader(data, byteSize);

	parser.proccessUpdatePacket(&reader, 20.0);

	unsigned int lastSeqIndex = historyBuffer.getLastSeqIndex();
	nsl::client::NetworkObject* o1 = objectManager.findObjectById(1);
	nsl::client::NetworkObject* o2 = objectManager.findObjectById(2);

	EXPECT_TRUE(NULL != o1);
	EXPECT_TRUE(NULL != o2);
}