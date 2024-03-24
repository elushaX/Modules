
#include "ObjectTests.hpp"

#include "core/Object.hpp"

#include "primitives/IntObject.hpp"

using namespace tp;
using namespace obj;

SUITE(Core) {
	TEST(BasicAPI) {
		objTestModule.initialize();

		{
			auto integer = objects_api::cast<IntObject>(NDO->create("int"));

			integer->val = 10;

			printf("%s\n", NDO->toString(integer).c_str());

			NDO->save(integer, "tmp.o");
			auto savedInt = NDO->load("tmp.o");

			printf("%s\n", NDO->toString(savedInt).c_str());

			CHECK(NDO->compare(integer, savedInt));
			CHECK(objects_api::cast<IntObject>(savedInt));
			CHECK(integer->val == objects_api::cast<IntObject>(savedInt)->val);

			NDO->destroy(integer);
			NDO->destroy(savedInt);
		}

		objTestModule.deinitialize();
	}
}