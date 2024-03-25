
#include "primitives/IntObject.hpp"

using namespace tp;
using namespace obj;

void IntObject::constructor(IntObject* self) { self->val = 0; }

void IntObject::copy(IntObject* self, const IntObject* in) { self->val = in->val; }

IntObject* IntObject::create(alni in) {
	auto out = objects_api::create<IntObject>();
	out->val = in;
	return out;
}

void IntObject::from_int(IntObject* self, alni in) { self->val = in; }

void IntObject::from_float(IntObject* self, alnf in) { self->val = (alni) in; }

void IntObject::from_string(IntObject* self, const std::string& in) { self->val = std::stol(in); }

std::string IntObject::to_string(IntObject* self) { return std::to_string(self->val); }

alni IntObject::to_int(IntObject* self) { return alni(self->val); }

alnf IntObject::to_float(IntObject* self) { return alnf(self->val); }

static alni save_size(IntObject* self) { return sizeof(alni); }

static void save(IntObject* self, ArchiverOut& file_self) { file_self << self->val; }

static void load(ArchiverIn& file_self, IntObject* self) { file_self >> self->val; }

struct ObjectTypeConversions IntObjectTypeConversions = {
	.from_int = (object_from_int) IntObject::from_int,
	.from_float = (object_from_float) IntObject::from_float,
	.from_string = (object_from_string) IntObject::from_string,
	.to_string = (object_to_string) IntObject::to_string,
	.to_int = (object_to_int) IntObject::to_int,
	.to_float = (object_to_float) IntObject::to_float,
};

void divide(IntObject* self, IntObject* in) { self->val /= in->val; }

void mul(IntObject* self, IntObject* in) { self->val *= in->val; }

void sub(IntObject* self, IntObject* in) { self->val -= in->val; }

void add(IntObject* self, IntObject* in) { self->val += in->val; }

struct ObjectTypeArithmetics IntObject::TypeAriphm = {
	.add = (object_add) add,
	.sub = (object_sub) sub,
	.mul = (object_mul) mul,
	.div = (object_div) divide,
};

struct obj::ObjectType obj::IntObject::TypeData = {
	.base = nullptr,
	.constructor = (object_constructor) IntObject::constructor,
	.destructor = nullptr,
	.copy = (object_copy) IntObject::copy,
	.size = sizeof(IntObject),
	.name = "int",
	.conversions = &IntObjectTypeConversions,
	.arithmetics = &IntObject::TypeAriphm,
	.save_size = (object_save_size) save_size,
	.save = (object_save) save,
	.load = (object_load) load,
};
