
#include "primitives/ClassObject.hpp"
#include "primitives/DictObject.hpp"
#include "primitives/NullObject.hpp"

using namespace tp;
using namespace obj;

void ClassObject::constructor(ClassObject* self) {
	self->members = objects_api::create<DictObject>();

	self->addMember(objects_api::getNull(), "__init__");
	self->addMember(objects_api::getNull(), "__del__");
}

void ClassObject::copy(ClassObject* self, const ClassObject* blueprint) {
	objects_api::copy(self->members, blueprint->members);
}

void ClassObject::destructor(ClassObject* self) { objects_api::destroy(self->members); }

void ClassObject::addMember(Object* obj, const std::string& id) { members->put(id, obj); }

void ClassObject::createMember(const std::string& type, const std::string& id) {
	auto newo = tp::obj::objects_api::createByName(type.c_str());
	members->put(id, newo);
}

alni ClassObject::save_size(ClassObject* self) {
	return sizeof(alni); // dict object address
}

void ClassObject::save(ClassObject* self, ArchiverOut& file_self) {
	// save dict object
	alni ndo_object_adress = objects_api::save(file_self, self->members);
	file_self << ndo_object_adress;
}

void ClassObject::load(ArchiverIn& file_self, ClassObject* self) {
	alni ndo_object_address;
	file_self >> ndo_object_address;
	self->members = objects_api::cast<DictObject>(objects_api::load(file_self, ndo_object_address));
	tp::obj::objects_api::increaseReferenceCount(self->members);
}

tp::Buffer<Object*> childs_retrival(ClassObject* self) {
	tp::Buffer<Object*> out;
	out.append(self->members);
	return out;
}

alni allocated_size(ClassObject* self) { return sizeof(DictObject*); }

alni allocated_size_recursive(ClassObject* self) {
	alni out = sizeof(DictObject*);
	out += objects_api::objsize_ram_recursive_util(self->members, self->members->type);
	return out;
}

struct ObjectType ClassObject::TypeData = {
	.base = nullptr,
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
	.allocated_size_recursive = (object_allocated_size_recursive) allocated_size_recursive,
};
