#pragma once

#include "core/Object.hpp"

namespace tp::obj {

	struct DictObject : Object {
		static ObjectType TypeData;
		static void copy(ObjectsContext* context, DictObject* self, const DictObject* in);
		static void destructor(ObjectsContext* context, DictObject* self);
		static void constructor(ObjectsContext* context, DictObject* self);

		static alni save_size(DictObject* self);
		static void save(ObjectsContext* context, DictObject* self, ArchiverOut& file_self);
		static void load(ObjectsContext* context, ArchiverIn& file_self, DictObject* self);
		static Buffer<Object*> childs_retrival(DictObject* self);
		static alni allocated_size(DictObject* self);
		static alni allocated_size_recursive(DictObject* self);

		void put(const std::string&, Object*);
		void remove(ObjectsContext* context, const std::string&);
		Object* get(const std::string&);
		Map<std::string, Object*>::Idx presents(const std::string&);
		Object* getSlotVal(Map<std::string, Object*>::Idx);

		[[nodiscard]] const Map<std::string, Object*>& getItems() const;

	private:
		Map<std::string, Object*> items;
	};
}