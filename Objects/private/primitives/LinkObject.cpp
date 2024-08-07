
#include "primitives/LinkObject.hpp"

using namespace tp;
using namespace obj;

void LinkObject::constructor(LinkObject* self) { self->link = nullptr; }

void LinkObject::destructor(LinkObject* self) {
	if (self->link) objects_api::destroy(self->link);
}

void LinkObject::copy(LinkObject* self, const LinkObject* in) { self->setLink(in->link); }

LinkObject* LinkObject::create(Object* in) {
	auto out = objects_api::create<LinkObject>();
	return out;
}

alni LinkObject::save_size(LinkObject* self) { return sizeof(alni); }

void LinkObject::save(LinkObject* self, ArchiverOut& file_self) {
	if (self->link != nullptr) {
		alni link_object_save_adress = objects_api::save(file_self, self->link);
		file_self << link_object_save_adress;
	} else {
		alni null = -1;
		file_self << null;
	}
}

void LinkObject::load(ArchiverIn& file_self, LinkObject* self) {

	alni saved_object_adress;
	file_self >> saved_object_adress;

	if (saved_object_adress == -1) {
		self->link = nullptr;
	} else {
		self->link = objects_api::load(file_self, saved_object_adress);
		objects_api::increaseReferenceCount(self->link);
	}
}

tp::Buffer<Object*> LinkObject::childs_retrival(LinkObject* self) {
	tp::Buffer<Object*> out;
	if (self->link) out.append(self->link);
	return out;
}

alni LinkObject::allocated_size(LinkObject* self) { return sizeof(Object*); }

alni LinkObject::allocated_size_recursive(LinkObject* self) {
	alni out = sizeof(Object*);
	if (self->link) {
		out += objects_api::objsize_ram_recursive_util(self->link, self->link->type);
	}
	return out;
}

Object* LinkObject::getLink() { return link; }

void LinkObject::setLink(Object* obj) {
	if (link) objects_api::destroy(link);
	if (obj) objects_api::increaseReferenceCount(obj);
	link = obj;
}

static auto tm_set = TypeMethod{
	.nameid = "set",
	.descr = "sets the link",
	.args = { { "target", nullptr } },
	.exec =
		[](const TypeMethod* tm) {
			auto const self = (LinkObject*) tm->self;
			auto const target = tm->args[0].obj;
			self->setLink(target);
		},
};

static auto tm_get = TypeMethod{
	.nameid = "get",
	.descr = "gets the link",
	.exec =
		[](const TypeMethod* tm) {
			auto const self = (LinkObject*) tm->self;
			auto link = self->getLink();
			if (link) {
				tm->ret.obj = link;
			}
		},
	.ret = { "the link", nullptr },
};

struct obj::ObjectType LinkObject::TypeData = {
	.base = nullptr,

	.constructor = (object_constructor) LinkObject::constructor,
	.destructor = (object_destructor) LinkObject::destructor,
	.copy = (object_copy) LinkObject::copy,
	.size = sizeof(LinkObject),
	.name = "link",

	.conversions = nullptr,

	.save_size = (object_save_size) LinkObject::save_size,
	.save = (object_save) LinkObject::save,
	.load = (object_load) LinkObject::load,

	.childs_retrival = (object_debug_all_childs_retrival) LinkObject::childs_retrival,
	.allocated_size = (object_allocated_size) LinkObject::allocated_size,
	.allocated_size_recursive = (object_allocated_size_recursive) LinkObject::allocated_size_recursive,

	.type_methods = {
		.methods = {
			&tm_set,
			&tm_get,
		},
	},
};