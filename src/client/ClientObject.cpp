#include "../../include/nslClient.h"
#include "NetworkObject.h"

namespace nsl {
	ClientObject::ClientObject(client::NetworkObject*)
		: networkObject(networkObject)
	{
	}

	ClientObject::~ClientObject(void)
	{
	}

	int ClientObject::getId(void)
	{
		return networkObject->getId();
	}

	int ClientObject::getObjectClassId(void)
	{
		return networkObject->getObjectClass()->getId();
	}

	template<class T> T::Type ClientObject::get(int attrId)
	{
		return networkObject->get<T>(attrId);
	}
};