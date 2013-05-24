/*
 * Copyright (C) 2013 Petr Smrcek
 * This file is originally a part of the Net Stalker Library
 * For conditions of distribution and use, see copyright notice in nsl.h
 */

#include <algorithm>
#include <cmath>
#include "ClientImpl.h"
#include "../../include/nslBitStream.h"
#include "NetworkObject.h"
#include "../ObjectClassDefinition.h"
using namespace std;

namespace nsl {
	namespace client {

		ClientImpl::ClientImpl(Client* userObject, unsigned short applicationId)
			: connection(applicationId), objectManager(&historyBuffer), protocolParser(&historyBuffer, &objectManager, &customMessageBuffer)
		{
			lastUpdateTime = 0;
			this->userObject = userObject;
			lastConnectionState = CLOSED;
		}

		ClientImpl::~ClientImpl(void)
		{
			connection.close();
		}

		void ClientImpl::close(void) 
		{
			connection.close();
			lastConnectionState = CLOSED;
			objectManager.reset();
			historyBuffer.reset();
			customMessageBuffer.reset();
		}

		void ClientImpl::open(const char* address, const char* port, const char* clientPort)
		{
			connection.open(address, port, clientPort, getTime());
		}

		void ClientImpl::registerObjectClass(ObjectClassDefinition* objectClass)
		{
			if (lastConnectionState != CLOSED) {
				throw Exception(NSL_EXCEPTION_USAGE_ERROR, "NSL: trying to register object class when connection already opened");
			}
			objectManager.registerObjectClass(objectClass);
		}

		ClientState ClientImpl::updateNetwork(double time)
		{
			// get current time
			double currentTime = (time == 0 ? getTime() : time);
			if (currentTime < lastUpdateTime) {
				throw Exception(NSL_EXCEPTION_USAGE_ERROR, "NSL: invalid time passed to updateNetwork");
			}

			// check connection status and try to connect / handshake, if neccessary
			lastConnectionState = connection.update(currentTime);
			if (lastConnectionState == CLOSED) {
				close();
				throw Exception(NSL_EXCEPTION_DISCONNECTED, "NSL: connection closed.");
			}
			
			if (lastConnectionState != CONNECTED) {
				switch(lastConnectionState) {
				case CONNECTING:
					return NSL_CS_CLOSED;
				case HANDSHAKING:
					return NSL_CS_HANDSHAKING;
				}
			}

			// proccess all incomming messages
			BitStreamReader* stream = connection.receive();
			try {
				while (stream != NULL) {
					protocolParser.proccessUpdatePacket(stream, currentTime);
					stream = connection.receive();
				}
			} catch (Exception e) {
				if (e.getCode() == NSL_EXCEPTION_DISCONNECTED) {
					close();
				}
				throw e;
			}

			// update application (if successful amount of data already arrived)
			bool enoughUpdatesBuffered = historyBuffer.getValidUpdatesCount() >= NSL_MINIMAL_PACKET_COUNT;
			if (enoughUpdatesBuffered) {

				int currentApplicationIndex = historyBuffer.getApplicationIndex();

				// count optimal application time
				double averageTickDuration = historyBuffer.getAverageTimeInterval(NSL_TIME_INTERVAL_AVERAGE_COUNT);
				double optimalApplicationTime = 
					historyBuffer.getTime(historyBuffer.getLastSeqIndex()) 
					- NSL_INTERPOLATION_LATENCY_PACKET_COUNT * averageTickDuration
					+ currentTime - historyBuffer.getLastUpdateApplicationTime();

				// if this is first application update, find first index to start and set first timeOverlap
				bool isFirst = false;
				if (currentApplicationIndex == NSL_UNDEFINED_BUFFER_INDEX) {
					currentApplicationIndex = historyBuffer.getFirstSeqIndex();
					if (currentApplicationIndex == NSL_UNDEFINED_BUFFER_INDEX) {
						throw Exception(NSL_EXCEPTION_LIBRARY_ERROR, "NSL: application first update failed, first data snapshot was not found");
					}

					timeOverlap = optimalApplicationTime - currentTime;
					isFirst = true;

				} else {
					// count the difference between optimal timeOverlap and current timeOverlap
					double timeOverlapDiff = optimalApplicationTime - currentTime - timeOverlap;

					// limit the difference by maximal speedup / slowdown
					double step = min(std::abs(timeOverlapDiff), (currentTime - lastUpdateTime) * (NSL_MAXIMAL_SPEEDUP - 1));

					// alter the timeOverlap
					timeOverlap += (timeOverlapDiff < 0) ? -step : step;
				}

				double updateTime = currentTime + timeOverlap;

				if (!historyBuffer.updateApplicationIndex(updateTime)) {
					throw Exception(NSL_EXCEPTION_LIBRARY_ERROR, "NSL: application time is no longer in buffer");
				}

				int targetApplicationIndex = historyBuffer.getApplicationIndex();
				
				// if this is not first update, index must be shifted, because current index has been already proccessed in previous update
				if (!isFirst) {
					if (currentApplicationIndex == targetApplicationIndex) {
						currentApplicationIndex = NSL_UNDEFINED_BUFFER_INDEX;
					} else {
						currentApplicationIndex = historyBuffer.getNextValidIndex(currentApplicationIndex);
					}
				}

				objectManager.applicationUpdate(currentApplicationIndex, targetApplicationIndex, userObject, updateTime);
				
				objectManager.deleteOldObjects();	// TODO: more effective would be calling this before applicationUpdate
			}
			
			// check state once more
			// disconnect could have occured during proccessing incomming messages
			lastConnectionState = connection.update(currentTime);
			if (lastConnectionState == CLOSED) {
				close();
				throw Exception(NSL_EXCEPTION_DISCONNECTED, "NSL: not connected.");
			}

			lastUpdateTime = currentTime;

			return enoughUpdatesBuffered ? NSL_CS_OPENED : NSL_CS_BUFFERING;
		}

		void ClientImpl::flushNetwork(void)
		{
			if (!historyBuffer.isEmpty()) {
				Packet* packet = connection.createPacket();
				BitStreamWriter* stream = packet->getStream();
				stream->write<Attribute<seqNumber> >(historyBuffer.indexToSeq(historyBuffer.getLastSeqIndex()));

				// custom message part
				customMessageBuffer.addSeq();
				int currentIndex = customMessageBuffer.getCurrentSeqIndex();
				seqNumber cmseq = customMessageBuffer.indexToSeq(currentIndex);
				stream->write<Attribute<seqNumber> >(cmseq);

				// add unacked reliable custom messages
				int index = customMessageBuffer.getFirstUnackedIndex();
				
				while (index != currentIndex) {
					protocolParser.pushBufferedMessagesByIndex(stream, index);
					index = customMessageBuffer.getNextValidIndex(index);
				}

				// new messages (current index)
				if (!newCustomMessages.empty()) {
					index = customMessageBuffer.getCurrentSeqIndex();
					stream->write<Attribute<seqNumber> >(customMessageBuffer.indexToSeq(index));
					for (std::vector<std::pair<BitStreamWriter*, bool> >::iterator it = newCustomMessages.begin(); it != newCustomMessages.end(); it++) {

						// append data to stream
						unsigned int size;
						byte* data = it->first->toBytes(size);
						/*if (size > NSL_MAX_CUSTOM_MESSAGE_SIZE) {
							throw Exception(NSL_EXCEPTION_USAGE_ERROR, "NSL: maximal custom message size exceeded");
						}*/ //Cannot happen, buffer has limited size
						stream->write<Attribute<customMessageSizeNumber> >(size);
						stream->write(size, data);

						// store message if it is realiable
						if (it->second) {
							customMessageBuffer.addBufferedMessage(data, size);
						}

						delete it->first;
					}
					newCustomMessages.clear();
					stream->write<Attribute<customMessageSizeNumber> >(0);
				}

				packet->send();
				delete packet;
			}
		}

		BitStreamWriter* ClientImpl::createCustomMessage(bool reliable)
		{
			BitStreamWriter* stream = new BitStreamWriter(NSL_MAX_CUSTOM_MESSAGE_SIZE, true);
			newCustomMessages.push_back(std::pair<BitStreamWriter*, bool>(stream, reliable));
			return stream;
		}
	};
};
