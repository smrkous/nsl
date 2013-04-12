#include "../common.h"
#include "../../include/nslServer.h"
#include "NetworkObject.h"
#include "../ObjectClassDefinition.h"

namespace nsl {
	ServerObject::ServerObject(server::NetworkObject* networkObject)
	{
		this->networkObject = networkObject;
	}

	ServerObject::~ServerObject(void)
	{

	}

	unsigned int ServerObject::getId(void)
	{
		if (networkObject->getDestroyIndex() != NSL_UNDEFINED_BUFFER_INDEX) {
			throw Exception(NSL_EXCEPTION_USAGE_ERROR, "NSL: trying to work with inaccessible object.");
		}
		return networkObject->getId();
	}

	unsigned short ServerObject::getObjectClassId(void)
	{
		if (networkObject->getDestroyIndex() != NSL_UNDEFINED_BUFFER_INDEX) {
			throw Exception(NSL_EXCEPTION_USAGE_ERROR, "NSL: trying to work with inaccessible object.");
		}
		return networkObject->getObjectClass()->getId();
	}

	void ServerObject::set(unsigned int attrId, unsigned int byteSize, byte* value)
	{
		if (networkObject->getDestroyIndex() != NSL_UNDEFINED_BUFFER_INDEX) {
			throw Exception(NSL_EXCEPTION_USAGE_ERROR, "NSL: trying to work with inaccessible object.");
		}
		networkObject->set(attrId, byteSize, value);
	}

	void ServerObject::destroy(void)
	{
		if (networkObject->getDestroyIndex() != NSL_UNDEFINED_BUFFER_INDEX) {
			throw Exception(NSL_EXCEPTION_USAGE_ERROR, "NSL: trying to work with inaccessible object.");
		}
		networkObject->destroy();
	}
};