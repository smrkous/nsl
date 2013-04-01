#include "Connection.h"

namespace nsl {
	namespace client {
		Connection::Connection(short applicationId, short clientPort) 
			: applicationId(applicationId), clientPort(clientPort)
		{
			state = CLOSED;
			connectedAddress.sin_family = AF_INET;
			connectionId = 0;
		}

		void Connection::open(const char* address, int port, double time)
		{
			if (state != CLOSED) {
				throw Exception(NSL_EXCEPTION_USAGE_ERROR, "NSL: trying to open new connection on already connected or connecting client.");
			}

			// cleanup
			if (bufferedMessage != NULL) {
				delete bufferedMessage;
				bufferedMessage = NULL;
			}
			connectionId = 0;

			// try to estabilish connection
			socket.open(clientPort);

			// set address
			connectedAddress.sin_addr.s_addr = inet_addr( address );
			connectedAddress.sin_port = htons( (unsigned short) port );

			// send connection request
			state = CONNECTING;
			sendConnectionRequest(time);
		}

		void Connection::sendConnectionRequest(double time)
		{
			BitStreamWriter* stream = new BitStreamWriter(6);
			stream->write<uint16>(applicationId);
			stream->write<uint32>(0);
			socket.send(connectedAddress, stream->buffer, 6);
			lastRequest = time;
		}

		void Connection::sendHandshake(double time)
		{
			BitStreamWriter* stream = new BitStreamWriter(6);
			stream->write<uint16>(applicationId);
			stream->write<uint32>(connectionId);
			socket.send(connectedAddress, stream->buffer, 6);
			lastRequest = time;
		}

		void Connection::sendDisconnect(void)
		{
			BitStreamWriter* stream = new BitStreamWriter(7);
			stream->write<uint16>(applicationId);
			stream->write<uint32>(connectionId);
			stream->write<uint8>(NSL_FLAG_DISCONNECT);
			socket.send(connectedAddress, stream->buffer, 7);
		}

		ConnectionState Connection::update(double time)
		{
			// if CLOSED or CONNECTED, nothing happens
			// if CONNECTING or HANDSHAKING, incomming packets are proccessed for update
			switch(state) {
			case CONNECTING:
				sockaddr_in sender;
				int size;
				// accept only connectionResponse (and get its connectionId)
				while (size = socket.receive(sender,buffer,MAX_PACKET_SIZE)) {
					if (size != 6) {
						continue;
					}
					BitStreamReader* stream = new BitStreamReader(buffer, size);
					if (stream->read<uint16>() != applicationId) {
						continue;
					}
					connectionId = stream->read<int32>();
					state = HANDSHAKING;
					sendHandshake(time);
					return state;
				}
				
				if (lastRequest + NSL_TIMEOUT_CONNECTION_REQUEST < time) {
					sendConnectionRequest(time);
				}
				break;
			case HANDSHAKING:
				sockaddr_in sender;
				int size;
				// accept only handshakeResponse (and get its status)
				while (size = socket.receive(sender,buffer,MAX_PACKET_SIZE)) {
					if (size < 7) {
						continue;
					}
					BitStreamReader* stream = new BitStreamReader(buffer, size);
					if (stream->read<uint16>() != applicationId) {
						continue;
					}
					if (stream->read<uint32>() != connectionId) {
						continue;
					}

					// process handshake response
					switch (stream->readByte()) {
					case NSL_FLAG_DISCONNECT:
						state = CLOSED;
						return state;
					case NSL_FLAG_UPDATE:
						state = CONNECTED;
						bufferedMessage = stream;
						return state;
					default:
						throw Exception(NSL_EXCEPTION_LIBRARY_ERROR, "NSL: protocol error, received not expected data.");
					}
				}

				if (lastRequest + NSL_TIMEOUT_HANDSHAKE < time) {
					sendHandshake(time);
				}
				break;
			case CLOSED:
				sockaddr_in sender;
				int size;
				// throw away all incomming packets so they will not mess later
				while (size = socket.receive(sender,buffer,MAX_PACKET_SIZE)) {}
			default:
				break;
			}

			return state;
		}

		void Connection::close()
		{
			if (state == HANDSHAKING || state == CONNECTED) {
				state = CLOSED;
				sendDisconnect();
			}
		}

		Packet* Connection::createPacket(void)
		{
			if (state != CONNECTED)
			{
				throw Exception(NSL_EXCEPTION_USAGE_ERROR, "NSL: trying to create packet when not connected.");
			}

			BitStreamWriter* stream = new BitStreamWriter();
			stream->write<uint16>(applicationId);
			stream->write<uint32>(connectionId);

			return new Packet(this, stream);
		}

		void Connection::send(Packet* packet)
		{
			if (state != CONNECTED)
			{
				throw Exception(NSL_EXCEPTION_USAGE_ERROR, "NSL: trying to send packet when not connected.");
			}

			socket.send(
				connectedAddress, 
				packet->stream->buffer, 
				packet->stream->currentByte - packet->stream->buffer
			);
		}

		BitStreamReader* Connection::receive(void)
		{
			if (state != CONNECTED)
			{
				throw Exception(NSL_EXCEPTION_USAGE_ERROR, "NSL: trying to receive data when not connected.");
			}

			// if there is some buffered message, first return it
			if (bufferedMessage != NULL) {
				BitStreamReader* response = bufferedMessage;
				bufferedMessage = NULL;
				return bufferedMessage;
			}

			// accept relevant packets
			sockaddr_in sender;
			int size;
			while (size = socket.receive(sender,buffer,MAX_PACKET_SIZE)) {
				if (size < 7) {
					continue;
				}
				BitStreamReader* stream = new BitStreamReader(buffer, size);
				if (stream->read<uint16>() != applicationId) {
					continue;
				}
				if (stream->read<uint32>() != connectionId) {
					continue;
				}

				// process payoad
				switch (stream->readByte()) {
				case NSL_FLAG_DISCONNECT:
					state = CLOSED;
					return NULL;
				case NSL_FLAG_UPDATE:
					return stream;
				default:
					throw Exception(NSL_EXCEPTION_LIBRARY_ERROR, "NSL: protocol error, received not expected data.");
				}
			}

			return NULL;
		}

	};
};