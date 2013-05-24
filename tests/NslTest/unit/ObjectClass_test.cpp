#include "gtest/gtest.h"
#include "include/nslReflection.h"
#include "src/ObjectClassDefinition.h"
#include <iostream>
#include <string>

TEST(ObjectClass_Unit, defineClass) {
	enum nslTestIds {
		//DUMMY_1,
		ATTRIBUTE_1,
		ATTRIBUTE_2,
		ATTRIBUTE_3,
		//DUMMY_2,
		ATTRIBUTE_4,
		ATTRIBUTE_5,
		ATTRIBUTE_6,
		//DUMMY_3,
		//DUMMY_4,
		ATTRIBUTE_7,
		OBJECT_ID
	};

	nsl::ObjectClass oc(nslTestIds::OBJECT_ID);
	oc.defineAttribute<nsl::int8>(nslTestIds::ATTRIBUTE_1);
	oc.defineAttribute<nsl::int16>(nslTestIds::ATTRIBUTE_2);
	oc.defineAttribute<nsl::float32>(nslTestIds::ATTRIBUTE_3);
	oc.defineAttribute<nsl::double64>(nslTestIds::ATTRIBUTE_4);
	oc.defineAttribute<nsl::uint8>(nslTestIds::ATTRIBUTE_5);
	oc.defineAttribute<nsl::uint32>(nslTestIds::ATTRIBUTE_6);
	oc.defineAttribute<nsl::int16>(nslTestIds::ATTRIBUTE_7);

	nsl::ObjectClassDefinition ocd(oc);
	EXPECT_EQ(7, ocd.getAttributeCount());
	EXPECT_EQ(nslTestIds::OBJECT_ID, ocd.getId());
	
	EXPECT_EQ(0, ocd.getDataOffset(nslTestIds::ATTRIBUTE_1));
	EXPECT_EQ(1, ocd.getDataOffset(nslTestIds::ATTRIBUTE_2));
	EXPECT_EQ(3, ocd.getDataOffset(nslTestIds::ATTRIBUTE_3));
	EXPECT_EQ(7, ocd.getDataOffset(nslTestIds::ATTRIBUTE_4));
	EXPECT_EQ(15, ocd.getDataOffset(nslTestIds::ATTRIBUTE_5));
	EXPECT_EQ(16, ocd.getDataOffset(nslTestIds::ATTRIBUTE_6));
	EXPECT_EQ(20, ocd.getDataOffset(nslTestIds::ATTRIBUTE_7));
}