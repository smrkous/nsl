/* 
	nslReflexion.h
	Contains classes and constructs required for object class and attribute definition.
*/

#pragma once

#include "nsl.h"
#include <vector>

namespace nsl {

	/// Typedef for abstract interpolation function
	typedef byte*(*interpolationFunctionPointer)(int, byte**, double*, double, byte*);


	/// Base class for all attributes
	/// It is strongly recommended to use only predefined basic attributes.
	/// However, it is possible to pass anything. Just make sure, that (byte *) cast works properly on T.
	template<class T>
	class NSL_IMPORT_EXPORT Attribute
	{
	public:
		/// Actual data type, which will be serialized and shared.
		typedef T Type;

		/// Return byte size of attribute
		static unsigned int getByteSize(void) {return sizeof(T);}

		/// Default interpolation function (linear). At least one point is provided always.
		static void defaultInterpolation(int pointCount, T** data, double* time, double targetTime, T* result) {
			
			if (pointCount == 1) {
				result = data[0];
				return;
			}

			// find time bounds
			int leftIndex = pointCount - 2;
			int rightIndex = pointCount - 1;
			while (leftIndex > 0 && time[leftIndex] > time) {
				rightIndex = leftIndex;
				leftIndex--;
			}

			result = data[leftIndex] + (data[rightIndex] - data[leftIndex])*(targetTime-time[leftIndex])/(time[rightIndex]-time[leftIndex]);
		}

		/// Get size of the arrays the interpolation function will receive (alias the number of interpolation points).
		//static unsigned int getDefaultInterpolationsPointCount(void) {return 2;}

		/// Should this type be interpolated by default?
		static bool isInterpolated(void) {return true;}
	};


	/// Predefined basic attributes

	struct NSL_IMPORT_EXPORT uint2 : Attribute<unsigned char>{};	// TODO: optimize
	struct NSL_IMPORT_EXPORT int8 : Attribute<char>{};
	struct NSL_IMPORT_EXPORT uint8 : Attribute<unsigned char>{};
	struct NSL_IMPORT_EXPORT int16 : Attribute<short>{};
	struct NSL_IMPORT_EXPORT uint16 : Attribute<unsigned short>{};
	#ifdef IS_64_BIT
	struct NSL_IMPORT_EXPORT int32 : Attribute<int>{};
	struct NSL_IMPORT_EXPORT uint32 : Attribute<unsigned int>{};
	#else
	struct NSL_IMPORT_EXPORT int32 : Attribute<long>{};
	struct NSL_IMPORT_EXPORT uint32 : Attribute<unsigned long>{};
	#endif
	struct NSL_IMPORT_EXPORT float32 : Attribute<float>{};
	struct NSL_IMPORT_EXPORT double64 : Attribute<double>{};


	/// Forward declaration for export
	//struct NSL_IMPORT_EXPORT AttributeDefinition;


	/// This struct is for inner usage only and should be not exported
	struct AttributeDefinition 
	{
		interpolationFunctionPointer interpolationFunction;
		//unsigned int interpolationPoints;
		unsigned int size;
		unsigned int identifier;
	};


	/// Base class for all objectClasses.
	/// Every class must have at least one attribute.
	class NSL_IMPORT_EXPORT ObjectClass
	{
	private:
		std::vector<AttributeDefinition> attributes;
		unsigned int attributeMaxId;
		unsigned int id;
	public:
		friend class ObjectClassDefinition;

		/// Create new object class with unique id.
		ObjectClass(unsigned int classId);

		/// Define attribute with default interpolation
		template <class T>
		void defineAttribute(unsigned int attrId);

		/// Define attribute with custom interpolation.
		/// If NULL is passed instead if interpolation function, no interpolation will occure.
		/// Size of the arrays the interpolation function will receive (number of interpolation points) can be specified by the last parameter.
		template <class T>
		void defineAttribute(unsigned int attrId, interpolationFunctionPointer interpolationFunction/*, unsigned int interpolationPoints = 2*/);
	};
	
};
