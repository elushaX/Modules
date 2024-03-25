
#include "ObjectTests.hpp"

#include "compiler/Functions.hpp"
#include "interpreter/Interpreter.hpp"
#include "primitives/MethodObject.hpp"

using namespace tp;
using namespace obj;

SUITE(PrimitiveObjects) {
	TEST(Dict) {
		objTestModule.initialize();

		{
			auto integer = objects_api::create<IntObject>();
			integer->val = 10;

			auto dict = objects_api::create<DictObject>();

			dict->put("val", integer);

			objects_api::save(dict, "dict.o");

			auto dictLoaded = objects_api::cast<DictObject>(objects_api::load("dict.o"));

			REQUIRE CHECK(dictLoaded->presents("val").isValid());
			CHECK(objects_api::cast<IntObject>(dictLoaded->get("val"))->val == 10);

			objects_api::destroy(dict);
			objects_api::destroy(dictLoaded);
		}

		objTestModule.deinitialize();
	}
}
