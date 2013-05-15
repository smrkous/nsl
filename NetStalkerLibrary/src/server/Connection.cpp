#include "Connection.h"
#include "../../include/nslBitStream.h"

namespace nsl {
	namespace server {

		/* Packet */

		Packet::Packet(Connection* connection, BitStreamWriter* stream, PeerConnection* peer) 
				: connection(connection), stream(stream), peer(peer)
		{
			
		}

		Packet::~Packet(void) 
		{
			delete stream;
		}

		void Packet::send(void) 
		{
			connection->send(this);
		}
		
		BitStreamWriter* Packet::getStream(void) 
		{
			return stream;
		}
			


		/* Connection */

		Connection::Connection(unsigned short applicationId) 
			: applicationId(applicationId), serverPort(serverPort)
		{
			state = CLOSED;
			timeoutIteratorValid = false;
			lastConnectionId = 0;
		}

		Connection::~Connection(void)
		{
		}

		void Connection::open(const char* port)
		{
			if (state != CLOSED) {
				throw Exception(NSL_EXCEPTION_USAGE_ERROR, "NSL: trying to open new connection on already connected server.");
			}

			// try start accepting connections
			socket.open(port);

			state = OPENED;
		}

		bool Connection::isOpened(void)
		{
			return state == OPENED;
		}

		UpdateCode Connection::update(PeerConnection*& peer, BitStreamReader*& data, double time)
		{
			if (state == CLOSED) {
				throw Exception(NSL_EXCEPTION_LIBRARY_ERROR, "NSL: trying to update connection when server is not connected.");
			}

			timeoutIteratorValid = false;

			Address sender;
			unsigned int size;
			while (size = socket.receive(sender,buffer,NSL_MAX_UDP_PACKET_SIZE)) {

				// check first 6 bytes for application and connection id
				if (size < 6) {
					continue;
				}

				BitStreamReader* stream = new BitStreamReader(buffer, size);
				if (stream->read<uint16>() != applicationId) {
					delete stream;
					continue;
				}

				int connectionId = stream->read<uint32>();

				// new connection
				if (connectionId == 0) {
					peer = new PeerConnection(++lastConnectionId, sender);
					peer->lastResponse = time;
					handshakingPeers.insert(std::pair<unsigned int, PeerConnection*>(lastConnectionId, peer));
					sendHandshake(sender, lastConnectionId);
					delete stream;
					continue;
				}

				// try to find the peer
				std::map<unsigned int, PeerConnection*>::iterator it = connectedPeers.find(connectionId);
				if (it == connectedPeers.end()) {

					// try handshaking peers
					it = handshakingPeers.find(connectionId);
					if (it != handshakingPeers.end()) {

						peer = it->second;
						handshakingPeers.erase(connectionId);
						connectedPeers.insert(std::pair<unsigned int, PeerConnection*>(connectionId, peer));
						peer->lastResponse = time;
						data = stream;
						return PEER_CONNECT;
					}

					sendDisconnect(sender, connectionId);
					delete stream;
					continue;
				}

				peer = it->second;

				if (size >= 7) {
					byte flag = stream->read<uint8>();
					switch (flag) {
					case NSL_CONNECTION_FLAG_DISCONNECT:
						sendDisconnect(sender, connectionId);
						connectedPeers.erase(connectionId);
						data = stream;
						peer->lastResponse = time;
						return PEER_DISCONNECT;
					case NSL_CONNECTION_FLAG_UPDATE:
						data = stream;
						peer->lastResponse = time;
						return PEER_UPDATE;
					case NSL_CONNECTION_FLAG_HANDSHAKE:
						delete stream;
						continue;
					default:
						delete stream;
						throw Exception(NSL_EXCEPTION_LIBRARY_ERROR, "NSL: protocol error, unknown flag arrived");
					}
				}
			}

			timeoutIteratorValid = true;
			timeoutIteratorHandshaking = true;
			timeoutIterator = handshakingPeers.begin();
			return EMPTY;
		}

		void Connection::close()
		{
			if (state == OPENED) {
				state = CLOSED;
				for (std::map<unsigned int, PeerConnection*>::iterator it = handshakingPeers.begin(); it != handshakingPeers.end(); it++) {
					sendDisconnect(it->second->connectedAddress, it->second->connectionId);
				}
				for (std::map<unsigned int, PeerConnection*>::iterator it = connectedPeers.begin(); it != connectedPeers.end(); it++) {
					sendDisconnect(it->second->connectedAddress, it->second->connectionId);
				}
			}
		}

		void Connection::disconnect(PeerConnection* peer) 
		{
			sendDisconnect(peer->connectedAddress, peer->connectionId);
			connectedPeers.erase(peer->connectionId);
			delete peer;
		}

		PeerConnection* Connection::proccessTimeouts(double time)
		{
			if (!timeoutIteratorValid) {
				return NULL;
			}
			
			PeerConnection* peer;

			if (timeoutIteratorHandshaking) {

				while(timeoutIterator != handshakingPeers.end()) {
					peer = timeoutIterator->second;
					if (peer->lastResponse + NSL_TIMEOUT_SERVER_HANDSHAKE_KILL < time) {
						sendDisconnect(peer->connectedAddress, peer->connectionId);
						handshakingPeers.erase(timeoutIterator++);
						return peer;
					}
					if (peer->lastResponse + NSL_TIMEOUT_SERVER_HANDSHAKE_RESEND < time) {
						sendHandshake(timeoutIterator->second->connectedAddress, timeoutIterator->second->connectionId);
					}
					timeoutIterator++;
				}

				timeoutIterator = connectedPeers.begin();
				timeoutIteratorHandshaking = false;
			}

			if (!timeoutIteratorHandshaking) {

				while(timeoutIterator != connectedPeers.end()) {
					peer = timeoutIterator->second;
					if (peer->lastResponse + NSL_TIMEOUT_SERVER_CONNECTED_KILL < time) {
						sendDisconnect(peer->connectedAddress, peer->connectionId);
						connectedPeers.erase(timeoutIterator++);
						return peer;
					}
					timeoutIterator++;
				}
			}

			timeoutIteratorValid = false;
			return NULL;
		}

		Packet* Connection::createPacket(PeerConnection* peer)
		{
			if (state != OPENED)
			{
				throw Exception(NSL_EXCEPTION_USAGE_ERROR, "NSL: trying to create packet when not connected.");
			}

			BitStreamWriter* stream = new BitStreamWriter();
			stream->write<uint16>(applicationId);
			stream->write<uint32>(peer->connectionId);
			stream->write<uint8>(NSL_CONNECTION_FLAG_UPDATE);

			return new Packet(this, stream, peer);
		}

		void Connection::send(Packet* packet)
		{
			if (state != OPENED)
			{
				throw Exception(NSL_EXCEPTION_USAGE_ERROR, "NSL: trying to send packet when server is not connected.");
			}

			if (connectedPeers.find(packet->peer->connectionId) == connectedPeers.end()) {
				throw Exception(NSL_EXCEPTION_USAGE_ERROR, "NSL: trying to send packet to not connected peer.");
			}

#ifdef NSL_COMPRESS
			unsigned int streamByteSize = packet->stream->getByteSize();
			
			unsigned int bytesAfterCompression = compress(packet->stream->buffer + 7, compressBuffer + 7, streamByteSize - 7, NSL_MAX_UDP_PACKET_SIZE-7);
			
			/*if (bytesAfterCompression == 0 || bytesAfterCompression > streamByteSize - 7) {
				// compression failed to make socket smaller, send it raw
				send(
					packet->peer->connectedAddress, 
					packet->stream->buffer, 
					streamByteSize
				);
			} else {*/
				// send compressed update
				memcpy(compressBuffer, packet->stream->buffer, 6);
				compressBuffer[6] = NSL_CONNECTION_FLAG_COMPRESSED_UPDATE;
				send(
					packet->peer->connectedAddress, 
					compressBuffer, 
					bytesAfterCompression + 7
				);
			/*}*/
#else
			send(
				packet->peer->connectedAddress, 
				packet->stream->buffer, 
				packet->stream->currentByte - packet->stream->buffer
			);
#endif
		}

		void Connection::send(Address& address, byte* data, unsigned int dataSize)
		{
			if (dataSize > NSL_MAX_UDP_PACKET_SIZE) {
				throw Exception(NSL_EXCEPTION_USAGE_ERROR, "NSL: trying to send too much data, maximum UDP packet limits reached");
			}
			socket.send(
				address, 
				data, 
				dataSize
			);
		}

		void Connection::sendHandshake(Address& address, unsigned int connectionId)
		{
			BitStreamWriter* stream = new BitStreamWriter(6);
			stream->write<uint16>(applicationId);
			stream->write<uint32>(connectionId);
			socket.send(address, stream->buffer, 6);
		}

		void Connection::sendDisconnect(Address& address, unsigned int connectionId)
		{
			BitStreamWriter* stream = new BitStreamWriter(7);
			stream->write<uint16>(applicationId);
			stream->write<uint32>(connectionId);
			stream->write<uint8>(NSL_CONNECTION_FLAG_DISCONNECT);
			socket.send(address, stream->buffer, 7);
		}

	};
};
