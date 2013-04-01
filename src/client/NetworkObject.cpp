#include "NetworkObject.h";
namespace nsl {
	namespace client {

		NetworkObject::NetworkObject(ObjectClassDefinition* objectClass, ObjectManager* objectManager, int id) 
			: objectClass(objectClass), objectManager(objectManager), id(id)
		{
			for (short i = 0; i < PACKET_BUFFER_SIZE; i++) {
				data[i] = NULL;
			}

			clientObject = new ClientObject(this);
		}

		NetworkObject::~NetworkObject(void) 
		{
			for (short i = 0; i < PACKET_BUFFER_SIZE; i++) {
				if (data[i] != NULL) delete[] data[i];
			}

			if (clientObject != NULL) {
				delete clientObject;
			}
		}

		ObjectClassDefinition* NetworkObject::getObjectClass(void)
		{
			return objectClass;
		}

		byte* NetworkObject::getDataBySeqIndex(int seqIndex)
		{
			return data[seqIndex];
		}

		void NetworkObject::setDataBySeqIndex(int seqIndex, byte* newData)
		{
			data[seqIndex] = newData;
		}

		int NetworkObject::getId(void)
		{
			return id;
		}

		void NetworkObject::findInterpolationPoints(HistoryBuffer* historyBuffer, double applicationTime)
		{
			if (state == BEFORE_BIRTH && state == DESTROYED) {
				// no points required, object is inaccessible from application
				return;
			}

			time = applicationTime;

			interpolationPointsCount = 0;
			int index = historyBuffer->getLastSeqIndex();
			bool dataMet = false;
			while (index != UNDEFINED_BUFFER_INDEX || interpolationPointsCount != NSL_INTERPOLATION_MINIMAL_DATA_COUNT) {
				if (data[index] != NULL) {
					// fill the array from end, so the points will be chronologicaly ordered
					interpolationPointsCount++;
					interpolationPoints[NSL_INTERPOLATION_MINIMAL_DATA_COUNT - interpolationPointsCount] = data[index];
					pointTimes[NSL_INTERPOLATION_MINIMAL_DATA_COUNT - interpolationPointsCount] = historyBuffer->getTime(index);
					dataMet = true;
				} else if (dataMet) {
					// we dont have to iterate further, because we got before object creation
					return;
				}
				index = historyBuffer->getPreviousValidIndex(index);
			}
		}

		template<class T> T::Type NetworkObject::get(int attrId)
		{
			AttributeDefinition* attribute = objectClass->getAttributeDefinition(attrId);

			// if no interpolation
			if (attribute->interpolationFunction == NULL) {
				// return value from first data, that passed the time

				int index = NSL_INTERPOLATION_MINIMAL_DATA_COUNT - 1;
				int indexFirst = NSL_INTERPOLATION_MINIMAL_DATA_COUNT - interpolationPointsCount;
				while (index >= indexFirst && pointTimes[index] > time) {
					index--;
				}

				return interpolationPoints[index];
			}

			// interpolate
			T::type result;

			//T*(*interpolation)(int, T**, double*, double, T*) = (T*(*)(int, T**, double*, double, T*)) attribute->interpolationFunction;
			((T*(*)(int, T**, double*, double, T*)) attribute->interpolationFunction) (
				interpolationPointsCount,
				interpolationPoints + (NSL_INTERPOLATION_MINIMAL_DATA_COUNT - interpolationPointsCount),
				pointTimes + (NSL_INTERPOLATION_MINIMAL_DATA_COUNT - interpolationPointsCount),
				time,
				&result
			);

			return result;
		}
	};
};
