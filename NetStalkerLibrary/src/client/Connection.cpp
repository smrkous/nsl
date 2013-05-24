/*
 * Copyright (C) 2013 Petr Smrcek
 * This file is originally a part of the Net Stalker Library
 * For conditions of distribution and use, see copyright notice in nsl.h
 */

#include "Connection.h"
#include "../../include/nslBitStream.h"
#if !defined NSL_PLATFORM_WINDOWS
#include <arpa/inet.h>
#endif

namespace nsl {
	namespace client {

		/* packet */

		Packet::Packet(Connection* connection, BitStreamWriter* stream) 
				: connection(connection), stream(stream) 
		{
		
		}

		void Packet::send(void) 
		{
			connection->send(this);
		}

		BitStreamWriter* Packet::getStream(void)
		{
			return stream;
		}

		Packet::~Packet(void)
		{
			delete stream;
		}


		/* connection */

		Connection::Connection(unsigned short applicationId) 
			: applicationId(applicationId), clientPort(clientPort)
		{
			state = CLOSED;
			connectionId = 0;
			bufferedMessage = NULL;
			// create bitStream with statically allocated buffer, so do not bind it
			bufferStream = new BitStreamReader(buffer, NSL_MAX_UDP_PACKET_SIZE, false);

#ifdef NSL_COMPRESS
			decompressionBuffer = new byte[NSL_INITIAL_MAX_PACKET_SIZE];
			decompressionBufferSize = NSL_INITIAL_MAX_PACKET_SIZE;
#endif
		}

		Connection::~Connection(void)
		{
			delete bufferStream;
#ifdef NSL_COMPRESS
			delete decompressionBuffer;
#endif
		}

		void Connection::open(const char* host, const char* port, const char* clientPort, double time)
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
			if (!socket.getAddressFromStrings(connectedAddress, host, port)) {
				throw Exception(NSL_EXCEPTION_USAGE_ERROR, "NSL: given address cannot be used to create connection");
			}
			
			// send connection request
			state = CONNECTING;
			sendConnectionRequest(time);
		}

		void Connection::sendConnectionRequest(double time)
		{
			BitStreamWriter stream(6);
			stream.write<uint16>(applicationId);
			stream.write<uint32>(0);
			socket.send(connectedAddress, stream.buffer, 6);
			lastRequest = time;
		}

		void Connection::sendHandshake(double time)
		{
			BitStreamWriter stream(7);
			stream.write<uint16>(applicationId);
			stream.write<uint32>(connectionId);
			stream.write<uint8>(NSL_CONNECTION_FLAG_HANDSHAKE);
			socket.send(connectedAddress, stream.buffer, 7);
			lastRequest = time;
		}

		void Connection::sendDisconnect(void)
		{
			BitStreamWriter stream(7);
			stream.write<uint16>(applicationId);
			stream.write<uint32>(connectionId);
			stream.write<uint8>(NSL_CONNECTION_FLAG_DISCONNECT);
			socket.send(connectedAddress, stream.buffer, 7);
		}

		ConnectionState Connection::update(double time)
		{
			// if CLOSED or CONNECTED, nothing happens
			// if CONNECTING or HANDSHAKING, incomming packets are proccessed for update
			Address sender;
			int size;

			switch(state) {
			case CONNECTING:
				// accept only connectionResponse (and get its connectionId)
				while (size = socket.receive(sender,buffer, NSL_MAX_UDP_PACKET_SIZE)) {
					if (size != 6) {
						continue;
					}

					// reset buffer stream
					bufferStream->resetStream(size);
					if (bufferStream->read<uint16>() != applicationId) {
						continue;
					}
					connectionId = bufferStream->read<int32>();
					state = HANDSHAKING;
					sendHandshake(time);
					return state;
				}
				
				if (lastRequest + NSL_TIMEOUT_CLIENT_CONNECTION_REQUEST < time) {
					sendConnectionRequest(time);
				}
				break;
			case HANDSHAKING:
				// accept only handshakeResponse (and get its status)
				while (size = socket.receive(sender,buffer,NSL_MAX_UDP_PACKET_SIZE)) {
					if (size < 7) {
						continue;
					}

					// reset buffer stream
					bufferStream->resetStream(size);
					if (bufferStream->read<uint16>() != applicationId) {
						continue;
					}
					if (bufferStream->read<uint32>() != connectionId) {
						continue;
					}

					// process handshake response
					switch (bufferStream->readByte()) {
					case NSL_CONNECTION_FLAG_DISCONNECT:
						state = CLOSED;
						return state;
					case NSL_CONNECTION_FLAG_UPDATE:
						state = CONNECTED;
						bufferedMessage = bufferStream->createSubreader(size - 7, true);
						return state;
					case NSL_CONNECTION_FLAG_COMPRESSED_UPDATE:
						state = CONNECTED;
#ifdef NSL_COMPRESS
						bufferedMessage = decompressStream(bufferStream);
#else
						throw Exception(NSL_EXCEPTION_LIBRARY_ERROR, "NSL: Compressed update was received from server, but compression is turned of on client");
#endif
						return state;
					default:
						throw Exception(NSL_EXCEPTION_LIBRARY_ERROR, "NSL: protocol error, received not expected data.");
					}
				}

				if (lastRequest + NSL_TIMEOUT_CLIENT_HANDSHAKE < time) {
					sendHandshake(time);
				}
				break;
			case CLOSED:
				// throw away all incomming packets so they will not mess later
				while (size = socket.receive(sender,buffer,NSL_MAX_UDP_PACKET_SIZE)) {}
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
			stream->write<uint8>(NSL_CONNECTION_FLAG_UPDATE);

			return new Packet(this, stream);
		}

		void Connection::send(Packet* packet)
		{
			if (state != CONNECTED)
			{
				throw Exception(NSL_EXCEPTION_USAGE_ERROR, "NSL: trying to send packet when not connected.");
			}

			if (packet->stream->currentByte - packet->stream->buffer > NSL_MAX_UDP_PACKET_SIZE) {
				throw Exception(NSL_EXCEPTION_USAGE_ERROR, "NSL: trying to send too much data, maximum UDP packet limits reached");
			}
			
			socket.send(
				connectedAddress, 
				packet->stream->buffer, 
				packet->stream->currentByte - packet->stream->buffer
			);

			delete packet;
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
				return response;
			}

			// accept relevant packets
			Address sender;
			int size;
			while (size = socket.receive(sender,buffer,NSL_MAX_UDP_PACKET_SIZE)) {
				if (size < 7) {
					continue;
				}
				bufferStream->resetStream(size);
				if (bufferStream->read<uint16>() != applicationId) {
					continue;
				}
				if (bufferStream->read<uint32>() != connectionId) {
					continue;
				}

				// process payoad
				switch (bufferStream->readByte()) {
				case NSL_CONNECTION_FLAG_DISCONNECT:
					state = CLOSED;
					return NULL;
				case NSL_CONNECTION_FLAG_UPDATE:
					return bufferStream;//->createSubreader(size - 7, true);
				case NSL_CONNECTION_FLAG_COMPRESSED_UPDATE:
#ifdef NSL_COMPRESS
					return decompressStream(bufferStream);
#else
					throw Exception(NSL_EXCEPTION_LIBRARY_ERROR, "NSL: Compressed update was received from server, but compression is turned of on client");
#endif
				default:
					throw Exception(NSL_EXCEPTION_LIBRARY_ERROR, "NSL: protocol error, received not expected data.");
				}
			}

			return NULL;
		}

#ifdef NSL_COMPRESS
		BitStreamReader* Connection::decompressStream(BitStreamReader* input)
		{
			unsigned int streamByteSize = input->getRemainingByteSize();

			if (streamByteSize == 0) {
				return input;
			}

			unsigned int decompressedByteSize = decompress(input->currentByte, decompressionBuffer, streamByteSize, decompressionBufferSize);
			while (decompressedByteSize == 0) {
				delete decompressionBuffer;
				decompressionBufferSize *= 2;
				decompressionBuffer = new byte[decompressionBufferSize];
				decompressedByteSize = decompress(input->currentByte, decompressionBuffer, streamByteSize, decompressionBufferSize);
			}
			
			return new BitStreamReader(decompressionBuffer, decompressedByteSize, false);		
		}
#endif

	};
};
