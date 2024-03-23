
#include "core/Object.hpp"

#include "primitives/ClassObject.hpp"
#include "primitives/MethodObject.hpp"

#include "interpreter/Interpreter.hpp"

using namespace tp;
using namespace obj;

void obj::save_string(ArchiverOut& file, const std::string& string) {
	file << string.size();
	file.writeBytes(string.c_str(), string.size());
}

ualni obj::save_string_size(const std::string& string) {
	return string.size() + sizeof(string.size());
}

void obj::load_string(ArchiverIn& file, std::string& out) {
	typeof(out.size()) size;
	file >> size;
	auto buff = new char[size + 1];
	file.readBytes(buff, size);
	buff[size] = 0;
	out = buff;
	delete[] buff;
}

Object* ObjectMemAllocate(const ObjectType* type);
void ObjectMemDeallocate(Object* in);

objects_api* obj::NDO = nullptr;

void hierarchy_copy(Object* self, const Object* in, const ObjectType* type);
void hierarchy_construct(Object* self, const ObjectType* type);

objects_api::objects_api() {
	memSetVal(sl_callbacks, SAVE_LOAD_MAX_CALLBACK_SLOTS * sizeof(save_load_callbacks*), 0);
	interp = new Interpreter();
}

objects_api::~objects_api() { delete interp; }

void objects_api::define(ObjectType* type) {
	MODULE_SANITY_CHECK(gModuleObjects)

	DEBUG_ASSERT(NDO && "using uninitialized objects api")
	DEBUG_ASSERT(!types.presents(type->name) && "Type Redefinition")
	types.put(type->name, type);

	type->type_methods.init();
}

Object* objects_api::create(const std::string& name) {
	MODULE_SANITY_CHECK(gModuleObjects)

	const ObjectType* type = types.get(name);

	Object* obj_instance = ObjectMemAllocate(type);

	if (!obj_instance) {
		return nullptr;
	}

	hierarchy_construct(obj_instance, obj_instance->type);

	setReferenceCount(obj_instance, 0);

	NDO_CASTV(ClassObject, obj_instance, classobj);

	if (classobj) {
		auto idx = classobj->members->presents("__init__");
		DEBUG_ASSERT(idx)
		if (idx) {
			auto constructor_obj = classobj->members->getSlotVal(idx);
			NDO_CASTV(MethodObject, constructor_obj, constructor_method);
			if (constructor_method) {
				interp->execAll(constructor_method, classobj);
			}
		}
	}

	return obj_instance;
}

Object* objects_api::copy(Object* self, const Object* in) {
	if (self->type != in->type) {
		return nullptr;
	}

	hierarchy_copy(self, in, self->type);

	return self;
}

bool objects_api::compare(Object* first, Object* second) {
	if (first->type == second->type) {
		if (first->type->comparison) {
			return first->type->comparison(first, second);
		}

		// raw data comparison
		return memCompare(first, second, first->type->size) == 0;
	}

	return false;
}

Object* objects_api::instantiate(Object* in) {
	obj::Object* obj = NDO->create(in->type->name);
	NDO->copy(obj, in);
	return obj;
}

void objects_api::set(Object* self, alni val) {
	if (self->type->convesions && self->type->convesions->from_int) {
		self->type->convesions->from_int(self, val);
		return;
	}
}

void objects_api::set(Object* self, alnf val) {
	if (self->type->convesions && self->type->convesions->from_float) {
		self->type->convesions->from_float(self, val);
		return;
	}
}

void objects_api::set(Object* self, const std::string& val) {
	if (self->type->convesions && self->type->convesions->from_string) {
		self->type->convesions->from_string(self, val);
		return;
	}
}

alni objects_api::toInt(Object* self) {
	DEBUG_ASSERT(self->type->convesions && self->type->convesions->to_int)
	return self->type->convesions->to_int(self);
}

alnf objects_api::toFloat(Object* self) {
	DEBUG_ASSERT(self->type->convesions && self->type->convesions->to_float)
	return self->type->convesions->to_float(self);
}

bool objects_api::toBool(Object* self) {
	if (self->type->convesions && self->type->convesions->to_int) {
		return (bool) self->type->convesions->to_int(self);
	}
	return true;
}

std::string objects_api::toString(Object* self) {
	DEBUG_ASSERT(self->type->convesions && self->type->convesions->to_string)
	return self->type->convesions->to_string(self);
}

void objects_api::destroy(Object* in) const {

	if (!in) {
		return;
	}

	ObjectMemHead* mh = NDO_MEMH_FROM_NDO(in);
	if (mh->refc > 1) {
		mh->refc--;
		return;
	}

	NDO_CASTV(ClassObject, in, classobj);
	if (classobj) {
		auto idx = classobj->members->presents("__del__");
		DEBUG_ASSERT(idx)
		if (idx) {
			auto constructor_obj = classobj->members->getSlotVal(idx);
			NDO_CASTV(MethodObject, constructor_obj, constructor_method);
			if (constructor_method) {
				interp->execAll(constructor_method, classobj);
			}
		}
	}

	for (const ObjectType* iter = in->type; iter; iter = iter->base) {
		if (iter->destructor) {
			iter->destructor(in);
		}
	}

	ObjectMemDeallocate(in);
}

halni objects_api::getReferenceCount(Object* in) {
	ObjectMemHead* mh = NDO_MEMH_FROM_NDO(in);
	return (halni) mh->refc;
}

void objects_api::increaseReferenceCount(Object* in) {
	ObjectMemHead* mh = NDO_MEMH_FROM_NDO(in);
	mh->refc++;
}

void objects_api::setReferenceCount(Object* in, halni count) {
	ObjectMemHead* mh = NDO_MEMH_FROM_NDO(in);
	mh->refc = count;
}


void hierarchy_copy(Object* self, const Object* in, const ObjectType* type) {
	if (type->base) {
		hierarchy_copy(self, in, type->base);
	}

	if (type->copy) {
		type->copy(self, in);
	}
}

void hierarchy_construct(Object* self, const ObjectType* type) {
	if (type->base) {
		hierarchy_construct(self, type->base);
	}

	if (type->constructor) {
		type->constructor(self);
	}
}

Object* obj::ndo_cast(const Object* in, const ObjectType* to_type) {
	const ObjectType* typeIter = in->type;
	while (typeIter) {
		if (typeIter == to_type) {
			return (Object*) in;
		}
		typeIter = typeIter->base;
	}
	return nullptr;
}
