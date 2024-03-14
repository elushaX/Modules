
#include "ObjectTests.hpp"

#include "compiler/function.h"
#include "core/object.h"
#include "interpreter/interpreter.h"
#include "primitives/interpreterobject.h"
#include "primitives/linkobject.h"
#include "primitives/methodobject.h"

using namespace tp;
using namespace obj;

SUITE(PrimitiveObjects) {
	TEST(Dict) {
		objTestModule.initialize();

		{
			auto integer = NDO_CAST(IntObject, NDO->create("int"));
			integer->val = 10;

			auto dict = NDO_CAST(DictObject, NDO->create("dict"));

			dict->put("val", integer);

			NDO->save(dict, "dict.o");

			auto dictLoaded = NDO_CAST(DictObject, NDO->load("dict.o"));

			CHECK(dictLoaded->presents("val").isValid());
			CHECK(NDO_CAST(IntObject, dictLoaded->get("val"))->val == 10);

			NDO->destroy(dict);
			NDO->destroy(dictLoaded);
		}

		objTestModule.deinitialize();
	}
}
