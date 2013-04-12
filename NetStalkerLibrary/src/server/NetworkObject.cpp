#include "NetworkObject.h"
#include "HistoryBuffer.h"
#include "../ObjectClassDefinition.h"
#include "../../include/nslServer.h"

namespace nsl {
	namespace server {
		NetworkObject::NetworkObject(ObjectClassDefinition* objectClass, HistoryBuffer* historyBuffer, unsigned int id)
			: historyBuffer(historyBuffer), objectClass(objectClass), id(id)
		{
			for (unsigned int i = 0; i < NSL_PACKET_BUFFER_SIZE; i++) {
				data[i] = NULL;
			}

			serverObject = new ServerObject(this);
			destroyIndex = NSL_UNDEFINED_BUFFER_INDEX;
			creationIndex = historyBuffer->getCurrentSeqIndex();
		}

		NetworkObject::~NetworkObject(void)
		{
			for (unsigned int i = 0; i < NSL_PACKET_BUFFER_SIZE; i++) {
				if (data[i] != NULL) delete[] data[i];
			}

			if (serverObject != NULL) {
				delete serverObject;
			}
		}

		unsigned int NetworkObject::getId(void)
		{
			return id;
		}

		ServerObject* NetworkObject::getUserObject(void)
		{
			return serverObject;
		}

		void NetworkObject::destroy(void)
		{
			if (destroyIndex == NSL_UNDEFINED_BUFFER_INDEX) {
				destroyIndex = historyBuffer->getCurrentSeqIndex();
			}
		}

		int NetworkObject::getDestroyIndex(void)
		{
			return destroyIndex;
		}

		int NetworkObject::getCreationIndex(void)
		{
			return creationIndex;
		}

		void NetworkObject::invalidateCreationIndex(void)
		{
			creationIndex = NSL_UNDEFINED_BUFFER_INDEX;
		}

		ObjectClassDefinition* NetworkObject::getObjectClass(void)
		{
			return objectClass;
		}

		void NetworkObject::set(unsigned int attrId, unsigned int byteSize, byte* value)
		{
			memcpy(
				data[historyBuffer->getCurrentSeqIndex()] + objectClass->getDataOffset(attrId),
				value,
				byteSize
			);
		}

		byte* NetworkObject::getDataBySeqIndex(short seqIndex)
		{
			return data[seqIndex];
		}

		void NetworkObject::setDataBySeqIndex(short seqIndex, byte* data)
		{
			this->data[seqIndex] = data;
		}

	};
};