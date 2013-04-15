#include "NetworkObject.h"
#include "../../include/nslClient.h"
#include "../ObjectClassDefinition.h"

namespace nsl {
	ClientObject::ClientObject(client::NetworkObject* o)
		: networkObject(o)
	{
	}

	ClientObject::~ClientObject(void)
	{
	}

	unsigned int ClientObject::getId(void)
	{
		if (locked) {
			throw Exception(NSL_EXCEPTION_USAGE_ERROR, "NSL: trying to work with inaccessible object.");
		}
		return networkObject->getId();
	}

	unsigned short ClientObject::getObjectClassId(void)
	{
		if (locked) {
			throw Exception(NSL_EXCEPTION_USAGE_ERROR, "NSL: trying to work with inaccessible object.");
		}
		return networkObject->getObjectClass()->getId();
	}

	bool ClientObject::get(
		unsigned int attrId, 
		unsigned int byteSize, 
		unsigned int& pointCount,
		byte**& data,
		double*& times,
		double& targetTime,
		abstractInterpolationFunction& interpolationFunction
	) {
		if (locked) {
			throw Exception(NSL_EXCEPTION_USAGE_ERROR, "NSL: trying to work with inaccessible object.");
		}
		return networkObject->get(
			attrId, 
			byteSize, 
			pointCount,
			data,
			times,
			targetTime,
			interpolationFunction
		);
	}
};