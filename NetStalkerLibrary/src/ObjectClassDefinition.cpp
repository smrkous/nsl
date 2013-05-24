/*
 * Copyright (C) 2013 Petr Smrcek
 * This file is originally a part of the Net Stalker Library
 * For conditions of distribution and use, see copyright notice in nsl.h
 */

#include "ObjectClassDefinition.h"

namespace nsl {
	ObjectClassDefinition::ObjectClassDefinition(ObjectClass& userObject)
	{
		// at least one attribute is mandatory
		if (userObject.attributes.size() <= 0) {
			// TODO: throw new Exception
		}
			
		id = userObject.id;
		attributeCount = userObject.attributeMaxId + 1;	// if there are any ids skipped, space will be wasted
		attributes = new AttributeDefinition*[attributeCount];
		offsets = new unsigned int[attributeCount];
			
		std::vector<AttributeDefinition>::iterator it;
		for (it = userObject.attributes.begin(); it != userObject.attributes.end(); ++it ) {
			attributes[it->identifier] = &(*it);
		}

		unsigned int currentOffset = 0;
		for (unsigned int i = 0; i < attributeCount; i++) {
			if (attributes[i] != NULL) {
				offsets[i] = currentOffset;
				currentOffset += attributes[i]->size;
			}
		}
	}

	AttributeDefinition* ObjectClassDefinition::getAttributeDefinition(unsigned int attrId)
	{
		if (attrId >= attributeCount || attributes[attrId] == NULL) {
			// TODO throw exception
		}
		return attributes[attrId];
	}

	unsigned int ObjectClassDefinition::getAttributeCount(void)
	{
		return attributeCount;
	}

	unsigned int ObjectClassDefinition::getDataOffset(unsigned int attrId)
	{
		if (attrId >= attributeCount || attributes[attrId] == NULL) {
			// TODO throw exception
		}
		return offsets[attrId];
	}

	unsigned int ObjectClassDefinition::getByteSize(void)
	{
		// last attribute is always set
		return offsets[attributeCount-1] + attributes[attributeCount-1]->size;
	}

	unsigned short ObjectClassDefinition::getId(void)
	{
		return id;
	}
};