/* 
	nslReflexion.h
	Contains classes and constructs required for object class and attribute definition.
*/

#pragma once

#include "nsl.h"
#include <vector>
#include <algorithm>

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
	class NSL_IMPORT_EXPORT Attribute
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

	typedef Attribute<char> int8;
	typedef Attribute<unsigned char> uint8;
	typedef Attribute<short> int16;
	typedef Attribute<unsigned short> uint16;
	#ifdef IS_64_BIT
	typedef Attribute<int> int32;
	typedef Attribute<unsigned int> uint32;
	#else
	typedef Attribute<long> int32;
	typedef Attribute<unsigned long> uint32;
	#endif
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
		template <class T> NSL_IMPORT_EXPORT
		void defineAttribute(unsigned int attrId);

		/// Define attribute with custom interpolation.
		/// If NULL is passed instead if interpolation function, no interpolation will occure.
		template <class T> NSL_IMPORT_EXPORT
		void defineAttribute(unsigned int attrId, typename T::interpolationFunction interpolationFunction);
	};


	// Object Class templated methods implementation

	template <class T>
	void ObjectClass::defineAttribute(unsigned int attrId) {
		AttributeDefinition a;
		a.interpolationFunction = (typename T::isInterpolated() ? (abstractInterpolationFunction)typename T::defaultInterpolation : NULL);
		a.size = typename T::getByteSize();
		a.identifier = attrId;
		attributes.push_back(a);
		attributeMaxId = std::max(attributeMaxId, attrId);
	}

	template <class T>
	void ObjectClass::defineAttribute(unsigned int attrId, typename T::interpolationFunction interpolationFunction) {
		AttributeDefinition a;
		a.interpolationFunction = (abstractInterpolationFunction)interpolationFunction;
		a.size = typename T::getByteSize();
		a.identifier = attrId;
		attributes.push_back(a);
		attributeMaxId = std::max(attributeMaxId, attrId);
	}
	
};
