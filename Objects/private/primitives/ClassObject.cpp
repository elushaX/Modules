
#include "primitives/ClassObject.hpp"
#include "primitives/DictObject.hpp"
#include "primitives/NullObject.hpp"

using namespace tp;
using namespace obj;

void ClassObject::constructor(ObjectsContext* context, ClassObject* self) {
	self->members = NDO_CAST(DictObject, context->create("dict"));

	self->addMember(context->create<NullObject>(), "__init__");
	self->addMember(context->create<NullObject>(), "__del__");
}

void ClassObject::copy(ObjectsContext* context, ClassObject* self, const ClassObject* blueprint) { context->copy(self->members, blueprint->members); }

void ClassObject::destructor(ObjectsContext* context, ClassObject* self) { context->destroy(self->members); }

void ClassObject::addMember(Object* obj, const std::string& id) { members->put(id, obj); }

void ClassObject::createMember(ObjectsContext* context, const std::string& type, const std::string& id) {
	auto newObject = context->create(type);
	members->put(id, newObject);
}

alni ClassObject::save_size(ClassObject* self) {
	return sizeof(alni); // dict object adress
}

void ClassObject::save(ObjectsContext* context, ClassObject* self, ArchiverOut& file_self) {
	// save dictobject
	alni ndo_object_adress = context->save(file_self, self->members);
	file_self << ndo_object_adress;
}

void ClassObject::load(ObjectsContext* context, ArchiverIn& file_self, ClassObject* self) {
	alni ndo_object_adress;
	file_self >> ndo_object_adress;
	self->members = NDO_CAST(DictObject, context->load(file_self, ndo_object_adress));
	context->increaseReferenceCount(self->members);
}

tp::Buffer<Object*> childs_retrival(ClassObject* self) {
	tp::Buffer<Object*> out;
	out.append(self->members);
	return out;
}

alni allocated_size(ClassObject* self) { return sizeof(DictObject*); }

alni allocated_size_recursive(ClassObject* self) {
	// alni out = sizeof(DictObject*);
	// out += NDO->objsize_ram_recursive_util(self->members, self->members->type);
	return 0;
}

struct ObjectType ClassObject::TypeData = { .base = nullptr,
																						.constructor = (object_constructor) ClassObject::constructor,
																						.destructor = (object_destructor) ClassObject::destructor,
																						.copy = (object_copy) ClassObject::copy,
																						.size = sizeof(ClassObject),
																						.name = "class",
																						.save_size = (object_save_size) save_size,
																						.save = (object_save) save,
																						.load = (object_load) load,
																						.childs_retrival = (object_debug_all_childs_retrival) childs_retrival,
																						.allocated_size = (object_allocated_size) allocated_size,
																						.allocated_size_recursive = (object_allocated_size_recursive) allocated_size_recursive };
