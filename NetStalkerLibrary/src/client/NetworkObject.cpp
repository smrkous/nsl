#include "NetworkObject.h"
#include "../../include/nslClient.h"
#include "HistoryBuffer.h"
#include "../ObjectClassDefinition.h"
#include "ObjectManager.h"

namespace nsl {
	namespace client {

		NetworkObject::NetworkObject(ObjectClassDefinition* objectClass, ObjectManager* objectManager, unsigned int id) 
			: objectClass(objectClass), objectManager(objectManager), id(id)
		{
			for (unsigned int i = 0; i < NSL_PACKET_BUFFER_SIZE; i++) {
				data[i] = NULL;
				state[i] = EMPTY;
			}

			clientObject = new ClientObject(this);
			clientObject->locked = true;
			creationCustomMessage = NULL;
		}

		NetworkObject::~NetworkObject(void) 
		{
			for (unsigned int i = 0; i < NSL_PACKET_BUFFER_SIZE; i++) {
				if (data[i] != NULL) delete[] data[i];
			}

			if (clientObject != NULL) {
				delete clientObject;
			}

			if (creationCustomMessage != NULL) {
				delete creationCustomMessage;
			}
		}

		void NetworkObject::setCreationCustomMessage(BitStreamReader* reader)
		{
			if (creationCustomMessage != NULL) {
				delete creationCustomMessage;
			} 
			creationCustomMessage = reader;
		}

		BitStreamReader* NetworkObject::getCreationCustomMessage(void)
		{
			return creationCustomMessage;
		}

		ObjectClassDefinition* NetworkObject::getObjectClass(void)
		{
			return objectClass;
		}

		byte* NetworkObject::getDataBySeqIndex(int seqIndex)
		{
			return data[seqIndex];
		}

		void NetworkObject::setDataBySeqIndex(int seqIndex, byte* data, ObjectSnapshotMeta state, bool birth, bool death)
		{
			this->data[seqIndex] = data;
			this->state[seqIndex] = state;
			this->birth[seqIndex] = birth;
			this->death[seqIndex] = death;
		}

		ObjectSnapshotMeta NetworkObject::getStateBySeqIndex(int seqIndex)
		{
			return state[seqIndex];
		}

		ClientObject* NetworkObject::getClientObject(void)
		{
			return clientObject;
		}

		bool NetworkObject::getBirthBySeqIndex(int seqIndex)
		{
			return birth[seqIndex];
		}

		bool NetworkObject::getDeathBySeqIndex(int seqIndex)
		{
			return death[seqIndex];
		}

		unsigned int NetworkObject::getId(void)
		{
			return id;
		}

		void NetworkObject::findInterpolationPoints(HistoryBuffer* historyBuffer, int applicationIndex, double applicationTime)
		{
			if (getStateBySeqIndex(applicationIndex) == EMPTY) {
				// no points required, object is inaccessible from application
				return;
			}

			time = applicationTime;

			// middle point
			interpolationPointsCount = 1;
			interpolationPoints[NSL_INTERPOLATION_MINIMAL_DATA_COUNT] = data[applicationIndex];
			pointTimes[NSL_INTERPOLATION_MINIMAL_DATA_COUNT] = historyBuffer->getTime(applicationIndex);

			// forward search
			int index = applicationIndex;
			int finalIndex = historyBuffer->getLastSeqIndex();
			if (index != finalIndex) {
				index = historyBuffer->getNextValidIndex(index);
				while (index != NSL_UNDEFINED_BUFFER_INDEX && interpolationPointsCount <= NSL_INTERPOLATION_MINIMAL_DATA_COUNT) {
					if (data[index] != NULL) {
						interpolationPoints[NSL_INTERPOLATION_MINIMAL_DATA_COUNT + interpolationPointsCount] = data[index];
						pointTimes[NSL_INTERPOLATION_MINIMAL_DATA_COUNT + interpolationPointsCount] = historyBuffer->getTime(index);
						interpolationPointsCount++;
					} else {
						// we cannot to iterate further, because we got after object destruction
						break;
					}
					index = historyBuffer->getNextValidIndex(index);
				}
			}

			// backward search
			index = historyBuffer->getPreviousValidIndex(applicationIndex);
			interpolationPointsOffset = NSL_INTERPOLATION_MINIMAL_DATA_COUNT;
			while (index != NSL_UNDEFINED_BUFFER_INDEX && interpolationPointsOffset > 0) {
				if (data[index] != NULL) {
					interpolationPointsOffset--;
					interpolationPointsCount++;
					interpolationPoints[interpolationPointsOffset] = data[index];
					pointTimes[interpolationPointsOffset] = historyBuffer->getTime(index);
				} else {
					// we cannot iterate further, because we got before object creation
					break;
				}
				index = historyBuffer->getPreviousValidIndex(index);
			}
		}

		bool NetworkObject::get(
			unsigned int attrId, 
			unsigned int byteSize, 
			unsigned int& pointCount,
			byte**& data,
			double*& times,
			double& targetTime,
			abstractInterpolationFunction& interpolationFunction
		) {
			AttributeDefinition* attribute = objectClass->getAttributeDefinition(attrId);

			if (attribute->size != byteSize) {
				throw Exception(NSL_EXCEPTION_USAGE_ERROR, "NSL: called getter and the target attribute are of different types");
			}

			// if no interpolation
			if (attribute->interpolationFunction == NULL) {
				data[0] = interpolationPoints[NSL_INTERPOLATION_MINIMAL_DATA_COUNT];
				return false;
			}

			unsigned int offset = objectClass->getDataOffset(attrId);

			for (unsigned int i = interpolationPointsOffset; i < interpolationPointsOffset + interpolationPointsCount; i++) {
				attributeInPoints[i] = interpolationPoints[i] + offset;
			}
			
			pointCount = interpolationPointsCount;
			data = attributeInPoints + interpolationPointsOffset;
			times = pointTimes + interpolationPointsOffset;
			targetTime = time;
			interpolationFunction = attribute->interpolationFunction;

			return true;
		}

		void NetworkObject::beforeApplicationCreate(void)
		{
			clientObject->locked = false;
		}

		void NetworkObject::afterApplicationDestroy(void)
		{
			clientObject->locked = true;
		}
	};
};
