

#include "primitives/StringObject.hpp"

using namespace tp;
using namespace obj;

void StringObject::constructor(StringObject* self) { new (&self->val) std::string(); }

void StringObject::destructor(StringObject* self) { self->val.~basic_string(); }

void StringObject::copy(StringObject* self, const StringObject* in) { self->val = in->val; }

StringObject* StringObject::create(const std::string& in) {
	auto out = objects_api::cast<StringObject>(NDO->create("str"));
	out->val = in;
	return out;
}

void StringObject::from_int(StringObject* self, alni in) {
	// self->val = in;
}

void StringObject::from_float(StringObject* self, alnf in) {
	// self->val = in;
}

void StringObject::from_string(StringObject* self, const std::string& in) {
	// self->val = in;
}

std::string StringObject::to_string(StringObject* self) { return self->val; }

alni StringObject::to_int(StringObject* self) { return std::stoi(self->val); }

alnf StringObject::to_float(StringObject* self) { return std::stof(self->val); }

static alni save_size(StringObject* self) { return save_string_size(self->val); }

static void save(StringObject* self, ArchiverOut& file_self) { save_string(file_self, self->val); }

static void load(ArchiverIn& file_self, StringObject* self) {
	new (&self->val) std::string();
	load_string(file_self, self->val);
}

alni allocated_size(StringObject* self) {
	// return self->val.sizeAllocatedMem();
	return 0;
}

static bool compare_strings(StringObject* left, StringObject* right) { return left->val == right->val; }

struct ObjectTypeConversions StringObjectTypeConversions = {
	.from_int = (object_from_int) StringObject::from_int,
	.from_float = (object_from_float) StringObject::from_float,
	.from_string = (object_from_string) StringObject::from_string,
	.to_string = (object_to_string) StringObject::to_string,
	.to_int = (object_to_int) StringObject::to_int,
	.to_float = (object_to_float) StringObject::to_float,
};

struct obj::ObjectType StringObject::TypeData = {
	.base = nullptr,
	.constructor = (object_constructor) StringObject::constructor,
	.destructor = (object_destructor) StringObject::destructor,
	.copy = (object_copy) StringObject::copy,
	.size = sizeof(StringObject),
	.name = "str",
	.conversions = &StringObjectTypeConversions,
	.save_size = (object_save_size) save_size,
	.save = (object_save) save,
	.load = (object_load) load,
	.comparison = (object_compare) compare_strings,
	.allocated_size = (object_allocated_size) allocated_size,
};
