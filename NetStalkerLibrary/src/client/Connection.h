#pragma once

namespace nsl {
	class BitStreamReader;
	class BitStreamWriter;
};

#include "../configuration.h"
#include "../Socket.h"

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

			Packet(Connection* connection, BitStreamWriter* stream);
		public:
			void send(void);
			BitStreamWriter* getStream(void);
			~Packet(void);
		};


		class Connection 
		{
		private:
			friend class Packet;

			unsigned int connectionId;
			unsigned short applicationId;
			unsigned short clientPort;
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
			Connection(unsigned short applicationId, unsigned short clientPort);
			~Connection(void);
			void open(const char* address, unsigned short port, double time);
			ConnectionState update(double time);
			void close(void);
			Packet* createPacket(void);
			BitStreamReader* receive(void);
		};
	};
};
