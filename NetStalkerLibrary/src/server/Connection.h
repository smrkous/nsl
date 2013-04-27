#pragma once

namespace nsl {
	class BitStreamReader;
	class BitStreamWriter;
};

#include "../configuration.h"
#include "../Socket.h"
#ifdef NSL_COMPRESS
#include "../lz4/lz4.h"
#endif
#include <map>

namespace nsl {
	namespace server {

		enum ConnectionState {
			OPENED, CLOSED
		};

		enum UpdateCode {
			EMPTY, PEER_CONNECT, PEER_DISCONNECT, PEER_UPDATE
		};

		struct PeerConnection 
		{
			unsigned int connectionId;
			Address connectedAddress;
			double lastResponse;
			PeerConnection(unsigned int connectionId, Address& address)
				: connectionId(connectionId), connectedAddress(address) {}
		};


		class Connection;


		class Packet
		{
		private:
			friend class Connection;

			Connection* connection;
			BitStreamWriter* stream;
			PeerConnection* peer;

			Packet(Connection* connection, BitStreamWriter* stream, PeerConnection* peer);
		public:
			void send(void);
			BitStreamWriter* getStream(void);
			~Packet(void);
		};


		class Connection 
		{
		private:
			friend class Packet;

			unsigned short applicationId;
			unsigned short serverPort;
			Socket socket;
			byte buffer[MAX_PACKET_SIZE];
			ConnectionState state;
			std::map<unsigned int,PeerConnection*> connectedPeers;
			std::map<unsigned int,PeerConnection*> handshakingPeers;
			unsigned int lastConnectionId;
			std::map<unsigned int, PeerConnection*>::iterator timeoutIterator;
			bool timeoutIteratorHandshaking;
			bool timeoutIteratorValid;

			void send(Packet* packet);
			void sendHandshake(Address& address, unsigned int connectionId);
			void sendDisconnect(Address& address, unsigned int connectionId);
		public:
			Connection(unsigned short applicationId);
			~Connection(void);

			/// open connection
			void open(const char* port);

			bool isOpened(void);

			/// proccesses incoming packets
			/// connection requests are handled automaticcaly
			/// after succesful handshake, delete or update, this function returs the peer and payload packet data
			/// id EMPTY code is returned, no more packet are pending
			UpdateCode update(PeerConnection*& peer, BitStreamReader*& data, double time);

			/// if some peer timeouts for a long time, it is disconnected (and returned)
			/// callable only after update(...) returned EMPTY (otherwise nothing happens and NULL is returned)
			/// if NULL is returned after update(...) returned EMPTY, all timeouts have been already checked and solved
			/// if handshaking client timeouts just for a while, handshake is resent
			PeerConnection* proccessTimeouts(double time);

			/// send disconnect packet to peer and closes its connection
			void disconnect(PeerConnection* peer);

			/// disconnect all peers and close connection
			void close(void);

			Packet* createPacket(PeerConnection* peer);
		};
	};
};
