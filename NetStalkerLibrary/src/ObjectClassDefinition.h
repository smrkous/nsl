/*
 * Copyright (C) 2013 Petr Smrcek
 * This file is originally a part of the Net Stalker Library
 * For conditions of distribution and use, see copyright notice in nsl.h
 */

#pragma once

#include "../include/nslReflection.h"

namespace nsl {

	/// Class describing objectClass optimized for attribute and data queries.
	/// For inner usage only.
	class ObjectClassDefinition
	{
	private: 
		AttributeDefinition** attributes;
		unsigned int* offsets;	// optimization - specific offset can be otherwise retrieved by iteration trough all previous attributes
		unsigned int attributeCount;
		unsigned short id;
	public:
		/// Convert user-defined ObjectClass to ObjectClassDefinition.
		/// At least one attribute must be set, otherwise an exception is returned.
		ObjectClassDefinition(ObjectClass& userObject);

		/// Get info about specific attribut.
		/// Invalid attrId causes exception being thrown.
		AttributeDefinition* getAttributeDefinition(unsigned int attrId);

		/// Get attribute count
		/// 0 to count-1 are all valid attrIds
		unsigned int getAttributeCount(void);

		/// Get data offset for specific attribut in serialized data buffer.
		/// Invalid attrId causes exception being thrown.
		unsigned int getDataOffset(unsigned int attrId);

		/// Get object total byte size
		unsigned int getByteSize(void);

		/// Get object class unique identifier
		unsigned short getId(void);
	};

};