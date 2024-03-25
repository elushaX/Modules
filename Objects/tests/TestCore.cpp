
#include "ObjectTests.hpp"
#include "core/Object.hpp"
#include "primitives/IntObject.hpp"

using namespace tp;
using namespace obj;

SUITE(Core) {
	TEST(BasicAPI) {
		objTestModule.initialize();

		{
			auto integer = objects_api::create<IntObject>();

			integer->val = 10;

			printf("%s\n", objects_api::toString(integer).c_str());

			objects_api::save(integer, "tmp.o");
			auto savedInt = objects_api::load("tmp.o");

			printf("%s\n", objects_api::toString(savedInt).c_str());

			CHECK(objects_api::compare(integer, savedInt));
			CHECK(objects_api::cast<IntObject>(savedInt));
			CHECK(integer->val == objects_api::cast<IntObject>(savedInt)->val);

			objects_api::destroy(integer);
			objects_api::destroy(savedInt);
		}

		objTestModule.deinitialize();
	}
}