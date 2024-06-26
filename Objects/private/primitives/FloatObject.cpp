#include "primitives/FloatObject.hpp"

using namespace tp;
using namespace obj;

void FloatObject::constructor(FloatObject* self) { self->val = 0; }

void FloatObject::copy(FloatObject* self, const FloatObject* in) { self->val = in->val; }

FloatObject* FloatObject::create(alnf in) {
	auto out = objects_api::create<FloatObject>();
	out->val = alnf(in);
	return out;
}

void FloatObject::from_int(FloatObject* self, alni in) { self->val = alnf(in); }

void FloatObject::from_float(FloatObject* self, alnf in) { self->val = in; }

void FloatObject::from_string(FloatObject* self, const std::string& in) { self->val = std::stof(in); }

std::string FloatObject::to_string(FloatObject* self) { return std::to_string(self->val); }

alni FloatObject::to_int(FloatObject* self) { return alni(self->val); }

alnf FloatObject::to_float(FloatObject* self) { return self->val; }

static alni save_size(FloatObject* self) { return sizeof(alnf); }

static void save(FloatObject* self, ArchiverOut& file_self) { file_self << self->val; }

static void load(ArchiverIn& file_self, FloatObject* self) { file_self >> self->val; }

struct ObjectTypeConversions FloatObjectTypeConversions = {
	.from_int = (object_from_int) FloatObject::from_int,
	.from_float = (object_from_float) FloatObject::from_float,
	.from_string = (object_from_string) FloatObject::from_string,
	.to_string = (object_to_string) FloatObject::to_string,
	.to_int = (object_to_int) FloatObject::to_int,
	.to_float = (object_to_float) FloatObject::to_float,
};

static void mul(FloatObject* self, FloatObject* in) { self->val *= in->val; }

static void sub(FloatObject* self, FloatObject* in) { self->val -= in->val; }

static void add(FloatObject* self, FloatObject* in) { self->val += in->val; }

static void divide(FloatObject* self, FloatObject* in) { self->val /= in->val; }

struct ObjectTypeArithmetics FloatObject::TypeAriphm = {
	.add = (object_add) add,
	.sub = (object_sub) sub,
	.mul = (object_mul) mul,
	.div = (object_div) divide,
};

struct obj::ObjectType obj::FloatObject::TypeData = {
	.base = nullptr,
	.constructor = (object_constructor) FloatObject::constructor,
	.destructor = nullptr,
	.copy = (object_copy) FloatObject::copy,
	.size = sizeof(FloatObject),
	.name = "float",
	.conversions = &FloatObjectTypeConversions,
	.arithmetics = &FloatObject::TypeAriphm,
	.save_size = (object_save_size) save_size,
	.save = (object_save) save,
	.load = (object_load) load,
};
