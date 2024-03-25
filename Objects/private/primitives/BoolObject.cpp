

#include "primitives/BoolObject.hpp"

using namespace tp;
using namespace obj;

void BoolObject::constructor(BoolObject* self) { self->val = false; }

void BoolObject::copy(BoolObject* self, const BoolObject* in) { self->val = in->val; }

BoolObject* BoolObject::create(bool in) {
	auto out = objects_api::cast<BoolObject>(NDO->create("bool"));
	out->val = alni(in);
	return out;
}

void BoolObject::from_int(BoolObject* self, alni in) { self->val = in; }

void BoolObject::from_float(BoolObject* self, alnf in) { self->val = alni(bool(in)); }

void BoolObject::from_string(BoolObject* self, const std::string& in) { self->val = in == "true" || in == "True"; }

std::string BoolObject::to_string(BoolObject* self) { return std::to_string(bool(self->val)); }

alni BoolObject::to_int(BoolObject* self) { return self->val; }

alnf BoolObject::to_float(BoolObject* self) { return alnf(bool(self->val)); }

static alni save_size(BoolObject* self) { return sizeof(alni); }

static void save(BoolObject* self, ArchiverOut& file_self) { file_self << self->val; }

static void load(ArchiverIn& file_self, BoolObject* self) { file_self >> self->val; }

struct ObjectTypeConversions BoolObjectTypeConversions = {
	.from_int = (object_from_int) BoolObject::from_int,
	.from_float = (object_from_float) BoolObject::from_float,
	.from_string = (object_from_string) BoolObject::from_string,
	.to_string = (object_to_string) BoolObject::to_string,
	.to_int = (object_to_int) BoolObject::to_int,
	.to_float = (object_to_float) BoolObject::to_float,
};

struct obj::ObjectType obj::BoolObject::TypeData = {
	.base = nullptr,
	.constructor = (object_constructor) BoolObject::constructor,
	.destructor = nullptr,
	.copy = (object_copy) BoolObject::copy,
	.size = sizeof(BoolObject),
	.name = "bool",
	.conversions = &BoolObjectTypeConversions,
	.save_size = (object_save_size) save_size,
	.save = (object_save) save,
	.load = (object_load) load,
};
