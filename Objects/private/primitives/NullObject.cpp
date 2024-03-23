

#include "primitives/NullObject.hpp"

using namespace tp;
using namespace obj;

obj::NullObject* obj::NdoNull_globalInstance = nullptr;
bool uninit_flag = false;

void NullObject::uninit() {
	uninit_flag = true;
	NDO->destroy(NdoNull_globalInstance);
	NdoNull_globalInstance = nullptr;
}

void NullObject::destructor(Object* self) { DEBUG_ASSERT(uninit_flag && "Only one the instance of NullObject exists and thus it can't be destroyed"); }

std::string to_string(NullObject* self) { return "nullptr"; }

alni to_int(NullObject* self) { return 0; }

alnf to_float(NullObject* self) { return 0; }

obj::TypeMethods* tm_construct() {
	auto out = new obj::TypeMethods();

	return out;
}

struct ObjectTypeConversions NullObjectTypeConversions = {
	.from_int = nullptr,
	.from_float = nullptr,
	.from_string = nullptr,
	.to_string = (object_to_string) to_string,
	.to_int = (object_to_int) to_int,
	.to_float = (object_to_float) to_float,
};

struct ObjectType NullObject::TypeData = {
	.base = nullptr,
	.constructor = nullptr,
	.destructor = NullObject::destructor,
	.copy = nullptr,
	.size = sizeof(NullObject),
	.name = "null",
	.convesions = &NullObjectTypeConversions,
};
