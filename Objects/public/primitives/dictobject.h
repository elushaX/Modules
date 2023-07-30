#pragma once

#include "core/object.h"

namespace obj {

	struct DictObject : Object {
		static ObjectType TypeData;
		static void copy(Object* self, const Object* in);
		static void destructor(Object* self);
		static void constructor(Object* self);

		static tp::alni save_size(DictObject* self);
		static void save(DictObject* self, ArchiverOut& file_self);
		static void load(ArchiverIn& file_self, DictObject* self);
		static tp::Buffer<Object*> childs_retrival(DictObject* self);
		static tp::alni allocated_size(DictObject* self);
		static tp::alni allocated_size_recursive(DictObject* self);

		void put(tp::String, Object*);
		void remove(tp::String);
		Object* get(tp::String);
		tp::Map<tp::String, Object*>::Idx presents(tp::String);
		Object* getSlotVal(tp::Map<tp::String, Object*>::Idx);

		const tp::Map<tp::String, Object*>& getItems() const;

	private:
		tp::Map<tp::String, Object*> items;
	};
};