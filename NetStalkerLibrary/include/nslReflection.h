/* 
	nslReflection.h
	Contains classes and constructs required for object class and attribute definition.
*/

#pragma once

#include "nsl.h"
#include <vector>
#include <algorithm>
#include <stdint.h>

namespace nsl {

	/////////////////////////////////////////////////////////////////////
	// Attribute
	/////////////////////////////////////////////////////////////////////

	/// Typedef for abstract interpolation function
	typedef void(*abstractInterpolationFunction)(unsigned int, byte const*const*, double const*, double, byte*);

	/// Base class for all attributes
	/// It is strongly recommended to use only predefined basic attributes.
	/// However, it is possible to pass anything - it will be cast using (byte*)&value.
	template<class T>
	class Attribute
	{
	public:
		/// Actual data type, which will be serialized and shared.
		typedef T Type;

		typedef void(*interpolationFunction)(unsigned int, T const*const*, double const*, double, T*);

		/// Return byte size of attribute
		static unsigned int getByteSize(void) {return sizeof(T);}

		/// Default interpolation function (linear). At least one point is provided always.
		static void defaultInterpolation(unsigned int pointCount, T const*const* data, double const* time, double targetTime, T* result) {
			
			if (pointCount == 1) {
				*result = *data[0];
				return;
			}

			// find time bounds
			int leftIndex = pointCount - 2;
			int rightIndex = pointCount - 1;
			while (leftIndex > 0 && time[leftIndex] > targetTime) {
				rightIndex = leftIndex;
				leftIndex--;
			}

			*result = *data[leftIndex] + (*data[rightIndex] - *data[leftIndex])*(targetTime-time[leftIndex])/(time[rightIndex]-time[leftIndex]);
		}

		/// Should this type be interpolated by default?
		static bool isInterpolated(void) {return true;}
	};


	/// Predefined basic attributes

	typedef Attribute<int8_t> int8;
	typedef Attribute<uint8_t> uint8;
	typedef Attribute<int16_t> int16;
	typedef Attribute<uint16_t> uint16;
	typedef Attribute<int32_t> int32;
	typedef Attribute<uint32_t> uint32;
	typedef Attribute<int64_t> int64;
	typedef Attribute<uint64_t> uint64;
	typedef Attribute<float> float32;
	typedef Attribute<double> double64;



	/////////////////////////////////////////////////////////////////////
	// ObjectClass
	/////////////////////////////////////////////////////////////////////

	/// Data structure containing definition of single attribute
	struct AttributeDefinition 
	{
		abstractInterpolationFunction interpolationFunction;
		unsigned int size;
		unsigned int identifier;
	};


	/// Class representing type of network object.
	/// Every class must have at least one attribute.
	class ObjectClass
	{
	private:
		std::vector<AttributeDefinition> attributes;
		unsigned int attributeMaxId;
		unsigned short id;

		friend class ObjectClassDefinition;
	public:
		/// Create new object class with unique id.
		NSL_IMPORT_EXPORT 
		ObjectClass(unsigned short classId) 
			: id(classId), attributeMaxId(0) {}

		/// Define attribute with default interpolation
		template <class T>
		void defineAttribute(unsigned int attrId);

		/// Define attribute with custom interpolation.
		/// If NULL is passed instead if interpolation function, no interpolation will occure.
		template <class T>
		void defineAttribute(unsigned int attrId, typename T::interpolationFunction interpolationFunction);
	};


	// Object Class templated methods implementation

	template <class T>
	void ObjectClass::defineAttribute(unsigned int attrId) {
		AttributeDefinition a;
		a.interpolationFunction = (T::isInterpolated() ? (abstractInterpolationFunction)(T::defaultInterpolation) : NULL);
		a.size = T::getByteSize();
		a.identifier = attrId;
		attributes.push_back(a);
		attributeMaxId = std::max(attributeMaxId, attrId);
	}

	template <class T>
	void ObjectClass::defineAttribute(unsigned int attrId, typename T::interpolationFunction interpolationFunction) {
		AttributeDefinition a;
		a.interpolationFunction = (abstractInterpolationFunction)interpolationFunction;
		a.size = T::getByteSize();
		a.identifier = attrId;
		attributes.push_back(a);
		attributeMaxId = std::max(attributeMaxId, attrId);
	}
	
};
