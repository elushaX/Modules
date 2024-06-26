
#include "core/Object.hpp"

#include "primitives/NullObject.hpp"
#include "primitives/ClassObject.hpp"
#include "primitives/MethodObject.hpp"

#include "interpreter/Interpreter.hpp"

using namespace tp;
using namespace obj;

ObjectsContext* obj::gObjectsContext = nullptr;

ObjectsContext::ObjectsContext() { interp = new Interpreter(); }

ObjectsContext::~ObjectsContext() { delete interp; }

void objects_api::initialize() {
	ASSERT(!gObjectsContext)
	gObjectsContext = new ObjectsContext();

	gObjectsContext->nullObject = create<NullObject>();
	objects_api::increaseReferenceCount(gObjectsContext->nullObject);
}

void objects_api::finalize() {
	destroy(gObjectsContext->nullObject);

	assertNoLeaks();

	ASSERT(gObjectsContext)
	delete gObjectsContext;
	gObjectsContext = nullptr;
}

void obj::save_string(ArchiverOut& file, const std::string& string) {
	file << string.size();
	file.writeBytes(string.c_str(), string.size());
}

ualni obj::save_string_size(const std::string& string) { return string.size() + sizeof(string.size()); }

void obj::load_string(ArchiverIn& file, std::string& out) {
	std::size_t size;
	file >> size;
	auto buff = new char[size + 1];
	file.readBytes(buff, size);
	buff[size] = 0;
	out = buff;
	delete[] buff;
}

Object* ObjectMemAllocate(const ObjectType* type);
void ObjectMemDeallocate(Object* object);

void hierarchy_copy(Object* self, const Object* in, const ObjectType* type);
void hierarchy_construct(Object* self, const ObjectType* type);

void objects_api::define(ObjectType* type) {
	MODULE_SANITY_CHECK(gModuleObjects)

	DEBUG_ASSERT(gObjectsContext && "using uninitialized objects api")
	DEBUG_ASSERT(!gObjectsContext->types.presents(type->name) && "Type Redefinition")
	gObjectsContext->types.put(type->name, type);

	type->type_methods.init();
}

Object* objects_api::create(const ObjectType* type) {
	ASSERT(type);

	Object* obj_instance = ObjectMemAllocate(type);

	if (!obj_instance) {
		return nullptr;
	}

	hierarchy_construct(obj_instance, obj_instance->type);

	setReferenceCount(obj_instance, 0);

	auto classobj = objects_api::cast<ClassObject>(obj_instance);

	if (classobj) {
		auto idx = classobj->members->presents("__init__");
		DEBUG_ASSERT(idx)
		if (idx) {
			auto constructor_obj = classobj->members->getSlotVal(idx);
			auto constructor_method = objects_api::cast<MethodObject>(constructor_obj);
			if (constructor_method) {
				gObjectsContext->interp->execAll(constructor_method, classobj);
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
		// TODO : reconsider
		return memCompare(first->type, second->type, first->type->size - sizeof(Object) + sizeof(void*)) == 0;
	}

	return false;
}

Object* objects_api::instantiate(Object* in) {
	obj::Object* obj = objects_api::createByName(in->type->name);
	tp::obj::objects_api::copy(obj, in);
	return obj;
}

void objects_api::set(Object* self, alni val) {
	if (self->type->conversions && self->type->conversions->from_int) {
		self->type->conversions->from_int(self, val);
		return;
	}
}

void objects_api::set(Object* self, alnf val) {
	if (self->type->conversions && self->type->conversions->from_float) {
		self->type->conversions->from_float(self, val);
		return;
	}
}

void objects_api::set(Object* self, const std::string& val) {
	if (self->type->conversions && self->type->conversions->from_string) {
		self->type->conversions->from_string(self, val);
		return;
	}
}

alni objects_api::toInt(Object* self) {
	DEBUG_ASSERT(self->type->conversions && self->type->conversions->to_int)
	return self->type->conversions->to_int(self);
}

alnf objects_api::toFloat(Object* self) {
	DEBUG_ASSERT(self->type->conversions && self->type->conversions->to_float)
	return self->type->conversions->to_float(self);
}

bool objects_api::toBool(Object* self) {
	if (self->type->conversions && self->type->conversions->to_int) {
		return (bool) self->type->conversions->to_int(self);
	}
	return true;
}

std::string objects_api::toString(Object* self) {
	DEBUG_ASSERT(self->type->conversions && self->type->conversions->to_string)
	return self->type->conversions->to_string(self);
}

void objects_api::destroy(Object* in) {

	if (!in) {
		return;
	}

	if (in->references > 1) {
		in->references--;
		return;
	}

	auto classobj = objects_api::cast<ClassObject>(in);
	if (classobj) {
		auto idx = classobj->members->presents("__del__");
		DEBUG_ASSERT(idx)
		if (idx) {
			auto constructor_obj = classobj->members->getSlotVal(idx);
			auto constructor_method = objects_api::cast<MethodObject>(constructor_obj);
			if (constructor_method) {
				gObjectsContext->interp->execAll(constructor_method, classobj);
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

void objects_api::addTypeToGroup(ObjectType* type, InitialierList<const char*> path, alni cur_arg) {
	gObjectsContext->type_groups.addType(type, path, cur_arg);
}

bool objects_api::isType(const char* name) { return gObjectsContext->types.presents(name).isValid(); }

const ObjectType* objects_api::getType(const char* name) { return gObjectsContext->types.get(name); }

NullObject* objects_api::getNull() { return gObjectsContext->nullObject; }

NullObject* objects_api::getNullReferenced() {
	increaseReferenceCount(gObjectsContext->nullObject);
	return gObjectsContext->nullObject;
}