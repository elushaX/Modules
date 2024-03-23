
#include "primitives/IntObject.hpp"
#include "primitives/ListObject.hpp"

using namespace obj;
using namespace tp;

void ListObject::constructor(Object* in) {
	NDO_CASTV(ListObject, in, self);
	new (&self->items) List<Object*>();
}

void ListObject::copy(Object* in, const Object* target) {
	NDO_CASTV(ListObject, in, self);
	NDO_CASTV(ListObject, target, src);

	destructor(in);

	for (auto item : src->items) {
		self->pushBack(NDO->instatiate(item.data()));
	}
}

void ListObject::destructor(Object* in) {
	NDO_CASTV(ListObject, in, self);
	for (auto item : self->items) {
		NDO->destroy(item.data());
	}
	self->items.removeAll();
}

alni ListObject::save_size(ListObject* self) {
	alni len = self->items.length();
	return (len + 1) * sizeof(alni);
}

void ListObject::save(ListObject* self, ArchiverOut& file_self) {
	alni len = self->items.length();
	file_self << len;

	for (auto item : self->items) {
		alni ndo_object_adress = NDO->save(file_self, item.data());
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
		self->pushBack(NDO->load(file_self, ndo_object_adress));
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
		// out += NDO->objsize_ram_recursive_util(item.data(), item->type);
	}
	// return out;
	return 0;
}

void ListObject::pushBack(Object* obj) {
	obj::NDO->refinc(obj);
	items.pushBack(obj);
}

void ListObject::pushFront(Object* obj) {
	obj::NDO->refinc(obj);
	items.pushFront(obj);
}

void ListObject::delNode(tp::List<Object*>::Node* node) {
	obj::NDO->destroy(node->data);
	items.deleteNode(node);
}

void ListObject::popBack() {
	auto obj = items.last();
	if (obj) obj::NDO->destroy(obj->data);
	items.popBack();
}

const tp::List<Object*>& ListObject::getItems() const { return items; }

struct obj::ObjectType obj::ListObject::TypeData = { .base = NULL,
																										 .constructor = ListObject::constructor,
																										 .destructor = ListObject::destructor,
																										 .copy = ListObject::copy,
																										 .size = sizeof(ListObject),
																										 .name = "list",
																										 .convesions = NULL,
																										 .save_size = (object_save_size) save_size,
																										 .save = (object_save) save,
																										 .load = (object_load) load,
																										 .childs_retrival = (object_debug_all_childs_retrival) childs_retrival,
																										 .allocated_size = (object_allocated_size) allocated_size,
																										 .allocated_size_recursive = (object_allocated_size_recursive) allocated_size_recursive };
