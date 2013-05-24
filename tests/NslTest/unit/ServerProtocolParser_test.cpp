#include "gtest/gtest.h"
#include "src/configuration.h"
#include "src/server/ProtocolParser.h"
#include "src/server/Connection.h"
#include "src/server/Peer.h"
#include "src/server/HistoryBuffer.h"
#include "src/server/ObjectManager.h"
#include "src/server/NetworkObject.h"
#include "src/ObjectClassDefinition.h"
#include <iostream>
#include <string>
#include <map>

TEST(ServerProtocolParser_Unit, createHeader) {
	
	nsl::server::HistoryBuffer historyBuffer;
	nsl::server::ProtocolParser parser(&historyBuffer);

	nsl::Address addr;
	nsl::server::PeerConnection pc(1, addr);
	nsl::server::Peer peer(&pc);
	std::map<unsigned int, nsl::server::Peer*> peers;
	peers.insert(std::pair<unsigned int, nsl::server::Peer*>(1, &peer));
	nsl::server::ObjectManager objectManager;
	historyBuffer.addSeq(25.0, &objectManager, &peers);


	nsl::BitStreamWriter writer;
	std::set<nsl::server::NetworkObject*> scope;
	parser.writeUpdateToPeer(&writer, &peer, scope, historyBuffer.getCurrentSeqIndex());

	EXPECT_EQ(15, writer.getByteSize());
}

TEST(ServerProtocolParser_Unit, parseNewObjects) {
	/*
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
	*/
}