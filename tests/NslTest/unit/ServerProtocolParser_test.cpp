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
	nsl::server::PeerConnection* pc = new nsl::server::PeerConnection(1, addr);
	nsl::server::Peer peer(pc);
	std::map<unsigned int, nsl::server::Peer*> peers;
	peers.insert(std::pair<unsigned int, nsl::server::Peer*>(1, &peer));
	nsl::server::ObjectManager objectManager;
	historyBuffer.addSeq(25.0, &objectManager, &peers);

	nsl::BitStreamWriter writer;
	std::set<nsl::server::NetworkObject*> scope;
	parser.writeUpdateToPeer(&writer, &peer, scope, historyBuffer.getCurrentSeqIndex());

	unsigned int size;
	nsl::byte* data = writer.toBytes(size);
	ASSERT_EQ(15, size);
	nsl::BitStreamReader reader(data, size, true);

	nsl::seqNumber seq = historyBuffer.indexToSeq(historyBuffer.getCurrentSeqIndex());
	EXPECT_EQ(seq, reader.read<nsl::Attribute<nsl::seqNumber> >());
	EXPECT_EQ(seq, reader.read<nsl::Attribute<nsl::seqNumber> >());
	EXPECT_EQ(25.0, reader.read<nsl::double64>());
	EXPECT_EQ(0, reader.read<nsl::Attribute<nsl::seqNumber> >());
}

TEST(ServerProtocolParser_Unit, addNewObjects) {
	
	nsl::server::HistoryBuffer historyBuffer;
	nsl::server::ProtocolParser parser(&historyBuffer);

	nsl::Address addr;
	nsl::server::PeerConnection* pc = new nsl::server::PeerConnection(1, addr);
	nsl::server::Peer peer(pc);
	std::map<unsigned int, nsl::server::Peer*> peers;
	peers.insert(std::pair<unsigned int, nsl::server::Peer*>(1, &peer));
	nsl::server::ObjectManager objectManager;
	nsl::ObjectClass oc(0);
	oc.defineAttribute<nsl::uint8>(0);
	oc.defineAttribute<nsl::uint32>(1);
	nsl::ObjectClassDefinition ocd(oc);
	historyBuffer.addSeq(25.0, &objectManager, &peers);

	std::set<nsl::server::NetworkObject*> scope;
	nsl::server::NetworkObject o(&ocd, &historyBuffer, 1);
	nsl::server::NetworkObject o2(&ocd, &historyBuffer, 2);
	scope.insert(&o);
	scope.insert(&o2);

	nsl::BitStreamWriter writer;
	parser.writeUpdateToPeer(&writer, &peer, scope, historyBuffer.getCurrentSeqIndex());

	unsigned int size;
	nsl::byte* data = writer.toBytes(size);
	ASSERT_EQ(39, size);
	nsl::BitStreamReader reader(data, size, true);

	nsl::seqNumber seq = historyBuffer.indexToSeq(historyBuffer.getCurrentSeqIndex());
	EXPECT_EQ(seq, reader.read<nsl::Attribute<nsl::seqNumber> >());
	EXPECT_EQ(seq, reader.read<nsl::Attribute<nsl::seqNumber> >());
	EXPECT_EQ(25.0, reader.read<nsl::double64>());
	EXPECT_EQ(0, reader.read<nsl::Attribute<nsl::seqNumber> >());
	nsl::ObjectFlags flag;
	reader >> flag;
	EXPECT_EQ(flag.action, NSL_OBJECT_FLAG_ACTION_CREATE);
	EXPECT_EQ(flag.scopeCreate, NSL_OBJECT_FLAG_SC_BIRTH);
	EXPECT_EQ(0, reader.read<nsl::uint16>());

	unsigned int expectedId = 3 - reader.read<nsl::uint32>();
	reader.skipBits(40);
	reader >> flag;
	EXPECT_EQ(flag.action, NSL_OBJECT_FLAG_ACTION_CREATE);
	EXPECT_EQ(flag.scopeCreate, NSL_OBJECT_FLAG_SC_BIRTH);
	EXPECT_EQ(0, reader.read<nsl::uint16>());
	EXPECT_EQ(expectedId, reader.read<nsl::uint32>());
	reader.skipBits(40);
	reader >> flag;
	EXPECT_EQ(flag.action, NSL_OBJECT_FLAG_ACTION_END_OF_SECTION);
	ASSERT_EQ(0, reader.getRemainingByteSize());
}