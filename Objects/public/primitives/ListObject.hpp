
#pragma once

#include "core/Object.hpp"
#include "List.hpp"

namespace tp::obj {

	enum ListMethods {
		LISTOBJECT_PUSH_BACK,
		LISTOBJECT_GET_LENGTH,
	};

	struct ListObject : Object {
		static ObjectType TypeData;
		static void constructor(ListObject* self);
		static void copy(ListObject* self, const ListObject* in);
		static void destructor(ListObject* self);

		static alni allocated_size_recursive(ListObject* self);
		static alni allocated_size(ListObject* self);
		static Buffer<Object*> childs_retrival(ListObject* self);
		static void load(ArchiverIn& file_self, ListObject* self);
		static void save(ListObject* self, ArchiverOut& file_self);
		static alni save_size(ListObject* self);

		[[nodiscard]] const List<Object*>& getItems() const;

		void pushBack(Object* obj);
		void pushFront(Object* obj);
		void popBack();
		void delNode(List<Object*>::Node*);

	private:
		List<Object*> items;
	};
}