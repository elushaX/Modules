
#include "primitives/enumobject.h"

#include <malloc.h>

using namespace obj;
using namespace tp;

void EnumObject::constructor(EnumObject* self) {
	self->active = 0;
	self->nentries = 0;
	self->entries = NULL;
}

void obj::EnumObject::destructor(EnumObject* self) {
	if (self->entries) free(self->entries);
}

void EnumObject::copy(EnumObject* self, const EnumObject* in) {
	if (self->entries) free(self->entries);
	self->active = in->active;
	self->nentries = in->nentries;

	self->entries = (alni*) malloc(self->nentries * ENV_ALNI_SIZE_B);
	tp::memCopy(self->entries, in->entries, self->nentries * ENV_ALNI_SIZE_B);
}

void obj::EnumObject::init(tp::InitialierList<const char*> list) {

	if (entries) free(entries);

	active = 0;
	nentries = (uhalni) list.size();
	entries = (alni*) malloc(nentries * ENV_ALNI_SIZE_B);
	tp::memSetVal(entries, nentries * ENV_ALNI_SIZE_B, 0);

	alni* entry = entries;
	for (auto elem : list) {

		alni len = tp::String::Logic::calcLength(elem);
		if (len > ENV_ALNI_SIZE_B - 1) len = ENV_ALNI_SIZE_B - 1;

		tp::memCopy(entry, elem, len);

		for (alni* chech_entry = entries; chech_entry != entry; chech_entry++) {
			DEBUG_ASSERT(tp::memCompare(chech_entry, entry, ENV_ALNI_SIZE_B) != 0);
		}

		entry++;
	}
}

const char* obj::EnumObject::getActiveName() { return getItemName(active); }

const char* obj::EnumObject::getItemName(tp::uhalni idx) {
	DEBUG_ASSERT(entries && idx >= 0 && idx < nentries);
	return (const char*) (entries + idx);
}

void EnumObject::from_int(EnumObject* self, alni in) {
	if (self->entries && in >= 0 && in < self->nentries) {
		self->active = uhalni(in);
	}
}

void EnumObject::from_float(EnumObject* self, alnf in) {
	if (self->entries && in >= 0 && in < self->nentries) {
		self->active = uhalni(in);
	}
}

void EnumObject::from_string(EnumObject* self, String in) {
	if (self->entries) {
		alni* entry = self->entries;
		for (uhalni i = 0; i < self->nentries; i++) {
			if (tp::String::Logic::isEqualLogic((const char*) entry, in.read())) {
				self->active = i;
			}
			entry += 1;
		}
	}
}

String EnumObject::to_string(EnumObject* self) {
	if (!self->entries) {
		return tp::String();
	}
	auto val = (const char*) (&self->entries[self->active]);
	return String(val);
}

alni EnumObject::to_int(EnumObject* self) {
	if (!self->entries) {
		return -1;
	}
	return alni(self->active);
}

alnf EnumObject::to_float(EnumObject* self) {
	if (!self->entries) {
		return -1;
	}
	return alnf(self->active);
}

static alni save_size(EnumObject* self) {
	if (!self->entries) {
		return sizeof(uhalni);
	}
	return sizeof(uhalni) + sizeof(uhalni) + sizeof(alni) * self->nentries;
}

static void save(EnumObject* self, ArchiverOut& file_self) {
	if (!self->entries) {
		uhalni empty_code = -1;
		file_self << empty_code;
		return;
	}
	file_self << self->active;
	file_self << self->nentries;
	file_self.writeBytes((tp::int1*) self->entries, self->nentries * ENV_ALNI_SIZE_B);
}

static void load(ArchiverIn& file_self, EnumObject* self) {
	file_self >> self->active;
	if (self->active == -1) {
		self->nentries = 0;
		self->entries = NULL;
		return;
	}
	file_self >> self->nentries;
	self->entries = (alni*) malloc(self->nentries * ENV_ALNI_SIZE_B);
	file_self.readBytes((tp::int1*) self->entries, self->nentries * ENV_ALNI_SIZE_B);
}

bool obj::EnumObject::compare(EnumObject* first, EnumObject* second) { return first->entries != NULL && second->entries != NULL && first->active == second->active; }

EnumObject* obj::EnumObject::create(tp::InitialierList<const char*> list) {
	auto enum_object = (EnumObject*) obj::NDO->create("enum");
	enum_object->init(list);
	return enum_object;
}

alni allocated_size(EnumObject* self) {
	alni out = sizeof(uhalni) * 2 + sizeof(tp::alni*);
	if (self->entries) {
		out += self->nentries * sizeof(alni) * 2;
	}
	return out;
}

struct ObjectTypeConversions EnumObjectTypeConversions = {
	.from_int = (object_from_int) EnumObject::from_int,
	.from_float = (object_from_float) EnumObject::from_float,
	.from_string = (object_from_string) EnumObject::from_string,
	.to_string = (object_to_string) EnumObject::to_string,
	.to_int = (object_to_int) EnumObject::to_int,
	.to_float = (object_to_float) EnumObject::to_float,
};

struct obj::ObjectType obj::EnumObject::TypeData = {
	.base = NULL,
	.constructor = (object_constructor) EnumObject::constructor,
	.destructor = (object_destructor) EnumObject::destructor,
	.copy = (object_copy) EnumObject::copy,
	.size = sizeof(EnumObject),
	.name = "enum",
	.convesions = &EnumObjectTypeConversions,
	.save_size = (object_save_size) save_size,
	.save = (object_save) save,
	.load = (object_load) load,
	.comparison = (object_compare) EnumObject::compare,
	.allocated_size = (object_allocated_size) allocated_size,
};
