#include "../include/nslReflexion.h"

namespace nsl {
	ObjectClass::ObjectClass(unsigned int classId)
		: id(classId), attributeMaxId(0)
	{}

	template <class T>
	void ObjectClass::defineAttribute(unsigned int attrId) {
		AttributeDefinition a = new AttributeDefinition;
		a.identifier = attrId;
		a.interpolate = (T::enableInterpolation() ? (interpolationFunctionPointer)T::defaultInterpolation : NULL);
		//a.interpolationPoints = T::DefaultInterpolationsPointCount();
		a.size = sizeof(T::Type);
		attributes.push_back(a);
		attributeMaxId = max(attributeMaxIdentifier, attrId);
	}

	template <class T>
	void ObjectClass::defineAttribute(unsigned int attrId, T::Type*(*)(T::Type**, double*, double) interpolationFunction/*, unsigned int interpolationPoints = 2*/) {
		AttributeDefinition a = new AttributeDefinition;
		a.identifier = attrId;
		a.interpolate = (interpolationFunctionPointer)interpolationFunction;
		//a.interpolationPoints = interpolationPoints;
		a.size = sizeof(T::Type);
		attributes.push_back(a);
		attributeMaxIdentifier = max(attributeMaxIdentifier, attrId);
	}
}
