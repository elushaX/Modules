

#include "primitives/NullObject.hpp"

using namespace tp;
using namespace obj;

std::string to_string(NullObject* self) { return "nullptr"; }

alni to_int(NullObject* self) { return 0; }

alnf to_float(NullObject* self) { return 0; }

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
	.destructor = nullptr,
	.copy = nullptr,
	.size = sizeof(NullObject),
	.name = "null",
	.conversions = &NullObjectTypeConversions,
};
