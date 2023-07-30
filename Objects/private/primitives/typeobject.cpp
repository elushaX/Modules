
#pragma once

#include "NewPlacement.hpp"
#include "primitives/typeobject.h"
#include "primitives/nullobject.h"

using namespace obj;
using namespace tp;

TypeObject* TypeObject::create(const ObjectType* type) {
	NDO_CASTV(TypeObject, NDO->create("typeobject"), out);
	out->mTypeRef = type;
	return out;
}

static alni save_size(TypeObject* self) {
	tp::String const nameid(self->mTypeRef->name);
	return nameid.size() + sizeof(nameid.size());
}

static void save(TypeObject* self, ArchiverOut& file_self) {
	tp::String const nameid(self->mTypeRef->name);
	file_self << nameid;
}

static void load(ArchiverIn& file_self, TypeObject* self) {
	tp::String nameid;
	file_self >> nameid;

	auto idx = NDO->types.presents(nameid);

	if (idx) {
		self->mTypeRef = NDO->types.getSlotVal(idx);
	}
	else {
		self->mTypeRef = &NullObject::TypeData;
	}
}

static alni allocated_size(TypeObject* self) {
	return sizeof(alni);
}

static void from_string(TypeObject* self, tp::String in) {
	auto idx = NDO->types.presents(in);

	if (idx) {
		self->mTypeRef = NDO->types.getSlotVal(idx);
	}
	else {
		self->mTypeRef = &NullObject::TypeData;
	}
}

static String to_string(TypeObject* self) {
	return self->mTypeRef->name;
}

bool comparator(TypeObject* left, TypeObject* right) {
	return left->mTypeRef == right->mTypeRef;
}

static struct ObjectTypeConversions conversions = {
	.from_string = (object_from_string) from_string,
	.to_string = (object_to_string) to_string,
};

struct obj::ObjectType TypeObject::TypeData = {
	.base = NULL,
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