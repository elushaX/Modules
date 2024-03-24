
#include "primitives/TypeObject.hpp"
#include "primitives/NullObject.hpp"

using namespace tp;
using namespace obj;

TypeObject* TypeObject::create(const ObjectType* type) {
	auto out = objects_api::cast<TypeObject>(NDO->create("typeobject"));
	out->mTypeRef = type;
	return out;
}

static alni save_size(TypeObject* self) { return save_string_size(self->mTypeRef->name); }

static void save(TypeObject* self, ArchiverOut& file_self) { save_string(file_self, self->mTypeRef->name); }

static void load(ArchiverIn& file_self, TypeObject* self) {
	std::string nameid;
	load_string(file_self, nameid);

	auto idx = NDO->types.presents(nameid);

	if (idx) {
		self->mTypeRef = NDO->types.getSlotVal(idx);
	} else {
		self->mTypeRef = &NullObject::TypeData;
	}
}

static alni allocated_size(TypeObject* self) { return sizeof(alni); }

static void from_string(TypeObject* self, const std::string& in) {
	auto idx = NDO->types.presents(in);

	if (idx) {
		self->mTypeRef = NDO->types.getSlotVal(idx);
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
	.convesions = &conversions,
	.save_size = (object_save_size) save_size,
	.save = (object_save) save,
	.load = (object_load) load,
	.comparison = (object_compare) comparator,
	.allocated_size = (object_allocated_size) allocated_size,
};
