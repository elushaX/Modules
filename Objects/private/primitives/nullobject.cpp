

#include "primitives/nullobject.h"

using namespace obj;
using namespace tp;

obj::NullObject* obj::NdoNull_globalInstance = NULL;
bool uninit_flag = false;

void NullObject::uninit() {
	uninit_flag = true;
	NDO->destroy(NdoNull_globalInstance);
	NdoNull_globalInstance = nullptr;
}

void NullObject::destructor(Object* self) { DEBUG_ASSERT(uninit_flag && "Only one the instance of NullObject exists and thus it can't be destroyed"); }

String to_string(NullObject* self) { return "NULL"; }

alni to_int(NullObject* self) { return 0; }

alnf to_float(NullObject* self) { return 0; }

obj::TypeMethods* tm_construct() {
	auto out = new obj::TypeMethods();

	return out;
}

struct ObjectTypeConversions NullObjectTypeConversions = {
	.from_int = NULL,
	.from_float = NULL,
	.from_string = NULL,
	.to_string = (object_to_string) to_string,
	.to_int = (object_to_int) to_int,
	.to_float = (object_to_float) to_float,
};

struct ObjectType NullObject::TypeData = {
	.base = NULL,
	.constructor = NULL,
	.destructor = NullObject::destructor,
	.copy = NULL,
	.size = sizeof(NullObject),
	.name = "null",
	.convesions = &NullObjectTypeConversions,
};
