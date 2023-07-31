
#include "NewPlacement.hpp"

#include "Testing.hpp"

#include "core/object.h"

#include "primitives/intobject.h"

using namespace tp;
using namespace obj;

TEST_DEF_STATIC(BasicAPI) {
	auto integer = NDO_CAST(IntObject, NDO->create("int"));

	integer->val = 10;

	printf("%s\n", NDO->toString(integer).read());

	NDO->save(integer, "tmp.o");
	auto savedInt = NDO->load("tmp.o");

	printf("%s\n", NDO->toString(savedInt).read());

	NDO->destroy(integer);
	NDO->destroy(savedInt);
}

TEST_DEF(Core) {
	testBasicAPI();
}