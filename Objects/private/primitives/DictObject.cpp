
#include "primitives/DictObject.hpp"
#include "primitives/StringObject.hpp"

using namespace tp;
using namespace obj;

void DictObject::constructor(DictObject* self) { new (&self->items) Map<std::string, Object*>(); }

void DictObject::copy(DictObject* self, const DictObject* src) {
	destructor(self);
	constructor(self);

	for (auto item : src->items) {
		auto instance = objects_api::instantiate(item->val);
		self->items.put(item->key, instance);
	}
}

void DictObject::destructor(DictObject* self) {
	for (auto item : self->items) {
		objects_api::destroy(item->val);
	}
	self->items.~Map();
}

alni DictObject::save_size(DictObject* self) {
	// calculate size needed
	alni save_size = 0;

	// number on entries
	save_size += sizeof(alni);

	for (auto item : self->items) {
		// string length
		save_size += save_string_size(item->key);

		// object file adress
		save_size += sizeof(alni);
	}

	return save_size;
}

void DictObject::save(DictObject* self, ArchiverOut& file_self) {

	// write size
	alni len = self->items.size();
	file_self << len;

	// save hashmap pairs
	for (auto item : self->items) {
		// item val
		alni ndo_object_adress = objects_api::save(file_self, item->val);
		file_self << ndo_object_adress;

		// item key
		save_string(file_self, item->key);
	}
}

void DictObject::load(ArchiverIn& file_self, DictObject* self) {
	new (&self->items) tp::Map<std::string, Object*>();

	alni len;
	file_self >> len;

	for (alni i = 0; i < len; i++) {

		// read val
		alni ndo_object_adress;
		file_self >> ndo_object_adress;
		Object* val = objects_api::load(file_self, ndo_object_adress);

		// read key value
		std::string key;
		load_string(file_self, key);

		// add to dictinary
		self->put(key, val);
	}
}

tp::Buffer<Object*> DictObject::childs_retrival(DictObject* self) {
	tp::Buffer<Object*> out;
	out.reserve(self->items.size());
	ualni i = 0;
	for (auto item : self->items) {
		out[i] = item->val;
		i++;
	}
	return out;
}

alni DictObject::allocated_size(DictObject* self) {
	ASSERT(false)
	// alni out = self->items.sizeAllocatedMem();
	for (auto item : self->items) {
		// out += item->key.sizeAllocatedMem();
	}
	// return out;
	return 0;
}

alni DictObject::allocated_size_recursive(DictObject* self) {
	alni out = allocated_size(self);
	for (auto item : self->items) {
		out += objects_api::objsize_ram_recursive_util(item->val, item->val->type);
	}
	return out;
}

void DictObject::put(const std::string& str, Object* obj) {
	DEBUG_ASSERT(obj);
	objects_api::increaseReferenceCount(obj);
	items.put(str, obj);
}

void DictObject::remove(const std::string& str) {
	auto idx = items.presents(str);
	if (idx) {
		objects_api::destroy(items.getSlotVal(idx));
		items.remove(str);
	}
}

Object* DictObject::get(const std::string& str) { return items.get(str); }

tp::Map<std::string, Object*>::Idx DictObject::presents(const std::string& str) { return items.presents(str); }

Object* DictObject::getSlotVal(tp::Map<std::string, Object*>::Idx idx) { return items.getSlotVal(idx); }

const tp::Map<std::string, Object*>& DictObject::getItems() const { return items; }

static auto tm_get = TypeMethod{
	.nameid = "get",
	.descr = "gets the object",
	.args = { { "str key", nullptr } },
	.exec =
		[](const TypeMethod* tm) {
			auto const self = (DictObject*) tm->self;
			auto str_key = tm->args[0].obj;

			auto key = objects_api::cast<StringObject>(str_key);
			ASSERT(key);

			auto idx = self->presents(key->val);
			if (idx) {
				tm->ret.obj = self->getSlotVal(idx);
			}
		},
	.ret = { "object", nullptr },
};

static auto tm_put = TypeMethod{
	.nameid = "put",
	.descr = "puts the object into the dictinary",
	.args = { { "key", nullptr }, { "object", nullptr } },
	.exec =
		[](const TypeMethod* tm) {
			auto const self = (DictObject*) tm->self;

			auto str_key = tm->args[0].obj;
			auto obj = tm->args[1].obj;

			auto key = objects_api::cast<StringObject>(str_key);
			ASSERT(key);

			self->put(key->val, obj);
		},
};

static auto tm_remove = TypeMethod{
	.nameid = "remove",
	.descr = "remove the object from the dictinary",
	.args = {
		{ "key", nullptr },
	},
	.exec = [](const TypeMethod* tm) {
		auto const self = (DictObject*)tm->self;

		auto str_key = tm->args[0].obj;
		
		auto key = objects_api::cast<StringObject>(str_key);
		ASSERT(key);

		self->remove(key->val);
	},
};

struct obj::ObjectType DictObject::TypeData = {
	.base = nullptr,
	.constructor = (object_constructor) DictObject::constructor,
	.destructor = (object_destructor) DictObject::destructor,
	.copy = (object_copy) DictObject::copy,
	.size = sizeof(DictObject),
	.name = "dict",
	.save_size = (object_save_size) DictObject::save_size,
	.save = (object_save) DictObject::save,
	.load = (object_load) DictObject::load,

	.childs_retrival = (object_debug_all_childs_retrival) DictObject::childs_retrival,
	.allocated_size = (object_allocated_size) DictObject::allocated_size,
	.allocated_size_recursive = (object_allocated_size_recursive) DictObject::allocated_size_recursive,

	.type_methods = {
		.methods = {
			&tm_put,
			&tm_remove,
			&tm_get,
		},
	},
};
