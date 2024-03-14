
#include "ObjectTests.hpp"

#include "core/object.h"

#include "primitives/intobject.h"

using namespace tp;
using namespace obj;

SUITE(Core) {
	TEST(BasicAPI) {
		objTestModule.initialize();

		{
			auto integer = NDO_CAST(IntObject, NDO->create("int"));

			integer->val = 10;

			printf("%s\n", NDO->toString(integer).read());

			NDO->save(integer, "tmp.o");
			auto savedInt = NDO->load("tmp.o");

			printf("%s\n", NDO->toString(savedInt).read());

			CHECK(NDO->compare(integer, savedInt));
			CHECK(NDO_CAST(IntObject, savedInt));
			CHECK(integer->val == NDO_CAST(IntObject, savedInt)->val);

			NDO->destroy(integer);
			NDO->destroy(savedInt);
		}

		objTestModule.deinitialize();
	}
}