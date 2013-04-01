#pragma once

#include "../common.h"
#include "../socket.h"
#include "../../include/nslBitStream.h"

#define NSL_FLAG_UPDATE 1
#define NSL_FLAG_DISCONNECT 2
#define NSL_TIMEOUT_CONNECTION_REQUEST 0.5
#define NSL_TIMEOUT_HANDSHAKE 0.5

namespace nsl {
	namespace client {
		
		
		enum ConnectionState {
			CLOSED, CONNECTING, HANDSHAKING, CONNECTED
		};


		class Connection;


		class Packet
		{
		private:
			friend class Connection;

			Connection* connection;
			BitStreamWriter* stream;

			Packet(Connection* connection, BitStreamWriter* stream) 
				: connection(connection), stream(stream) {}
		public:
			void send(void) {connection->send(this);}
			BitStreamWriter* getStream(void) {return stream;}
			~Packet(void) {delete stream;}
		};


		class Connection 
		{
		private:
			friend class Packet;

			int connectionId;
			short applicationId;
			short clientPort;
			ConnectionState state;
			Socket socket;
			sockaddr_in connectedAddress;
			double lastRequest;
			byte buffer[MAX_PACKET_SIZE];
			BitStreamReader* bufferedMessage;

			void send(Packet* packet);
			void sendConnectionRequest(double time);
			void sendHandshake(double time);
			void sendDisconnect(void);
		public:
			Connection(short applicationId, short clientPort);
			~Connection(void);
			void open(const char* address, int port, double time);
			ConnectionState update(double time);
			void close(void);
			Packet* createPacket(void);
			BitStreamReader* receive(void);
		};
	};
};