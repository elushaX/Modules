
#pragma once

#include "core/Object.hpp"

namespace tp::obj {

	struct LinkObject : Object {
		static ObjectType TypeData;
		static void constructor(Object* self);
		static void destructor(LinkObject* self);
		static void copy(Object* self, const Object* in);
		static LinkObject* create(Object* in);

		static alni save_size(LinkObject* self);
		static void save(LinkObject* self, ArchiverOut& file_self);
		static void load(ArchiverIn& file_self, LinkObject* self);
		static alni allocated_size(LinkObject* self);
		static alni allocated_size_recursive(LinkObject* self);
		static Buffer<Object*> childs_retrival(LinkObject* self);

		Object* getLink();
		void setLink(Object* obj);

	private:
		Object* link;
	};
}