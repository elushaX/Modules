
#include "ObjectTests.hpp"

#include "compiler/Functions.hpp"
#include "core/Object.hpp"
#include "interpreter/Interpreter.hpp"
#include "primitives/InterpreterObject.hpp"
#include "primitives/LinkObject.hpp"
#include "primitives/MethodObject.hpp"

using namespace tp;
using namespace obj;

SUITE(PrimitiveObjects) {
	TEST(Dict) {
		objTestModule.initialize();

		{
			auto integer = objects_api::cast<IntObject>(NDO->create("int"));
			integer->val = 10;

			auto dict = objects_api::cast<DictObject>(NDO->create("dict"));

			dict->put("val", integer);

			NDO->save(dict, "dict.o");

			auto dictLoaded = objects_api::cast<DictObject>(NDO->load("dict.o"));

			CHECK(dictLoaded->presents("val").isValid());
			CHECK(objects_api::cast<IntObject>(dictLoaded->get("val"))->val == 10);

			NDO->destroy(dict);
			NDO->destroy(dictLoaded);
		}

		objTestModule.deinitialize();
	}
}
