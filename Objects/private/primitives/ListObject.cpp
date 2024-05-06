
#include "primitives/IntObject.hpp"
#include "primitives/ListObject.hpp"

using namespace tp;
using namespace obj;

void ListObject::constructor(ListObject* self) { new (&self->items) List<Object*>(); }

void ListObject::copy(ListObject* in, const ListObject* target) {
	destructor(in);
	constructor(in);

	for (auto item : target->items) {
		in->pushBack(objects_api::instantiate(item.data()));
	}
}

void ListObject::destructor(ListObject* in) {
	for (auto item : in->items) {
		objects_api::destroy(item.data());
	}
	in->items.removeAll();
}

alni ListObject::save_size(ListObject* self) {
	alni len = self->items.length();
	return (len + 1) * sizeof(alni);
}

void ListObject::save(ListObject* self, ArchiverOut& file_self) {
	alni len = self->items.length();
	file_self << len;

	for (auto item : self->items) {
		alni ndo_object_adress = objects_api::save(file_self, item.data());
		file_self << ndo_object_adress;
	}
}

void ListObject::load(ArchiverIn& file_self, ListObject* self) {
	new (&self->items) tp::List<Object*>();

	alni len;
	file_self >> len;

	for (alni i = 0; i < len; i++) {
		alni ndo_object_adress;
		file_self >> ndo_object_adress;
		self->pushBack(objects_api::load(file_self, ndo_object_adress));
	}
}

tp::Buffer<Object*> ListObject::childs_retrival(ListObject* self) {
	tp::Buffer<Object*> out;
	out.reserve(self->items.length());
	ualni i = 0;
	for (auto item : self->items) {
		out[i] = item.data();
		i++;
	}
	return out;
}

alni ListObject::allocated_size(ListObject* self) {
	// return self->items.sizeAllocatedMem();
	return {};
}

alni ListObject::allocated_size_recursive(ListObject* self) {
	ASSERT(false)
	// alni out = self->items.sizeAllocatedMem();
	for (auto item : self->items) {
		// out += objects_api::objsize_ram_recursive_util(item.data(), item->type);
	}
	// return out;
	return 0;
}

void ListObject::pushBack(Object* obj) {
	obj::objects_api::increaseReferenceCount(obj);
	items.pushBack(obj);
}

void ListObject::pushFront(Object* obj) {
	obj::objects_api::increaseReferenceCount(obj);
	items.pushFront(obj);
}

void ListObject::delNode(tp::List<Object*>::Node* node) {
	obj::objects_api::destroy(node->data);
	items.deleteNode(node);
}

void ListObject::popBack() {
	auto obj = items.lastNode();
	if (obj) obj::objects_api::destroy(obj->data);
	items.popBack();
}

const tp::List<Object*>& ListObject::getItems() const { return items; }

struct obj::ObjectType obj::ListObject::TypeData = {
	.base = nullptr,
	.constructor = (object_constructor) ListObject::constructor,
	.destructor = (object_destructor) ListObject::destructor,
	.copy = (object_copy) ListObject::copy,
	.size = sizeof(ListObject),
	.name = "list",
	.conversions = nullptr,

	.save_size = (object_save_size) save_size,
	.save = (object_save) save,
	.load = (object_load) load,

	.childs_retrival = (object_debug_all_childs_retrival) childs_retrival,
	.allocated_size = (object_allocated_size) allocated_size,
	.allocated_size_recursive = (object_allocated_size_recursive) allocated_size_recursive,
};
