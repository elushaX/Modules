
#include "primitives/TypeObject.hpp"
#include "primitives/NullObject.hpp"

using namespace tp;
using namespace obj;

TypeObject* TypeObject::create(const ObjectType* type) {
	auto out = objects_api::create<TypeObject>();
	out->mTypeRef = type;
	return out;
}

static alni save_size(TypeObject* self) { return save_string_size(self->mTypeRef->name); }

static void save(TypeObject* self, ArchiverOut& file_self) { save_string(file_self, self->mTypeRef->name); }

static void load(ArchiverIn& file_self, TypeObject* self) {
	std::string name;
	load_string(file_self, name);

	if (objects_api::isType(name.c_str())) {
		self->mTypeRef = objects_api::getType(name.c_str());
	} else {
		self->mTypeRef = &NullObject::TypeData;
	}
}

static alni allocated_size(TypeObject* self) { return sizeof(alni); }

static void from_string(TypeObject* self, const std::string& in) {
	if (objects_api::isType(in.c_str())) {
		self->mTypeRef = objects_api::getType(in.c_str());
	} else {
		self->mTypeRef = &NullObject::TypeData;
	}
}

static std::string to_string(TypeObject* self) { return self->mTypeRef->name; }

bool comparator(TypeObject* left, TypeObject* right) { return left->mTypeRef == right->mTypeRef; }

static struct ObjectTypeConversions conversions = {
	.from_string = (object_from_string) from_string,
	.to_string = (object_to_string) to_string,
};

struct obj::ObjectType TypeObject::TypeData = {
	.base = nullptr,
	//.constructor = (object_constructor) TypeObject::constructor,
	.size = sizeof(TypeObject),
	.name = "typeobject",
	.conversions = &conversions,
	.save_size = (object_save_size) save_size,
	.save = (object_save) save,
	.load = (object_load) load,
	.comparison = (object_compare) comparator,
	.allocated_size = (object_allocated_size) allocated_size,
};
