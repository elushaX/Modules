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

		void put(const std::string&, Object*);
		void remove(const std::string&);
		Object* get(const std::string&);
		tp::Map<std::string, Object*>::Idx presents(const std::string&);
		Object* getSlotVal(tp::Map<std::string, Object*>::Idx);

		[[nodiscard]] const tp::Map<std::string, Object*>& getItems() const;

	private:
		tp::Map<std::string, Object*> items;
	};
};