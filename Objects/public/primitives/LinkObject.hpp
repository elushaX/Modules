
#pragma once

#include "core/Object.hpp"

namespace tp::obj {

	struct LinkObject : Object {
		static ObjectType TypeData;
		static void constructor(ObjectsContext* context, Object* self);
		static void destructor(ObjectsContext* context, LinkObject* self);
		static void copy(ObjectsContext* context, Object* self, const Object* in);

		LinkObject* set(Object* in);

		static alni save_size(LinkObject* self);
		static void save(ObjectsContext* context, LinkObject* self, ArchiverOut& file_self);
		static void load(ObjectsContext* context, ArchiverIn& file_self, LinkObject* self);
		static alni allocated_size(LinkObject* self);
		static alni allocated_size_recursive(LinkObject* self);
		static Buffer<Object*> childs_retrival(LinkObject* self);

		Object* getLink();
		void setLink(ObjectsContext* context, Object* obj);

	private:
		Object* link;
	};
}