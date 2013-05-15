#pragma once

namespace nsl {
	class BitStreamReader;
	class BitStreamWriter;
};

#include "../configuration.h"
#include "../Socket.h"
#ifdef NSL_COMPRESS
#include "../compression/compression.h"
#endif

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
			Address connectedAddress;
			double lastRequest;
			byte buffer[NSL_MAX_UDP_PACKET_SIZE];
#ifdef NSL_COMPRESS
			byte* decompressionBuffer;
			unsigned int decompressionBufferSize;
#endif
			BitStreamReader* bufferStream;	// stream over buffer to make reading of it easier
			BitStreamReader* bufferedMessage;

			void send(Packet* packet);
			void sendConnectionRequest(double time);
			void sendHandshake(double time);
			void sendDisconnect(void);
		public:
			Connection(unsigned short applicationId);
			~Connection(void);
			/// clientPort can be null, then it will be chosen automatically
			void open(const char* address, const char* port, const char* clientPort, double time);
			ConnectionState update(double time);
			void close(void);
			Packet* createPacket(void);
			BitStreamReader* receive(void);
#ifdef NSL_COMPRESS
			// all remaining data in stream will be decompressed
			// new reader with new buffer will be created (containing just decompressed data)
			BitStreamReader* decompressStream(BitStreamReader* stream);
#endif
		};
	};
};
