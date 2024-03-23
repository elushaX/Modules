
#include "primitives/IntObject.hpp"

using namespace tp;
using namespace obj;

void IntObject::constructor(Object* self) { NDO_CAST(IntObject, self)->val = 0; }

void IntObject::copy(IntObject* self, const IntObject* in) { self->val = in->val; }

IntObject* IntObject::create(alni in) {
	NDO_CASTV(IntObject, NDO->create("int"), out)->val = in;
	return out;
}

void IntObject::from_int(Object* self, alni in) { NDO_CAST(IntObject, self)->val = in; }

void IntObject::from_float(Object* self, alnf in) { NDO_CAST(IntObject, self)->val = (alni) in; }

void IntObject::from_string(Object* self, const std::string& in) {
	NDO_CAST(IntObject, self)->val = std::stol(in);
}

std::string IntObject::to_string(Object* self) { return std::to_string(NDO_CAST(IntObject, self)->val); }

alni IntObject::to_int(Object* self) { return alni(NDO_CAST(IntObject, self)->val); }

alnf IntObject::to_float(Object* self) { return alnf(NDO_CAST(IntObject, self)->val); }

static alni save_size(IntObject* self) { return sizeof(alni); }

static void save(IntObject* self, ArchiverOut& file_self) { file_self << self->val; }

static void load(ArchiverIn& file_self, IntObject* self) { file_self >> self->val; }

struct ObjectTypeConversions IntObjectTypeConversions = {
	.from_int = IntObject::from_int,
	.from_float = IntObject::from_float,
	.from_string = IntObject::from_string,
	.to_string = IntObject::to_string,
	.to_int = IntObject::to_int,
	.to_float = IntObject::to_float,
};

void divide(IntObject* self, IntObject* in) { self->val /= in->val; }

void mul(IntObject* self, IntObject* in) { self->val *= in->val; }

void sub(IntObject* self, IntObject* in) { self->val -= in->val; }

void add(IntObject* self, IntObject* in) { self->val += in->val; }

struct ObjectTypeAriphmetics IntObject::TypeAriphm = {
	.add = (object_add) add,
	.sub = (object_sub) sub,
	.mul = (object_mul) mul,
	.div = (object_div) divide,
};

struct obj::ObjectType obj::IntObject::TypeData = {
	.base = nullptr,
	.constructor = IntObject::constructor,
	.destructor = nullptr,
	.copy = (object_copy) IntObject::copy,
	.size = sizeof(IntObject),
	.name = "int",
	.convesions = &IntObjectTypeConversions,
	.ariphmetics = &IntObject::TypeAriphm,
	.save_size = (object_save_size) save_size,
	.save = (object_save) save,
	.load = (object_load) load,
};
