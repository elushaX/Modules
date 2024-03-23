
#pragma once

#include "core/Object.hpp"

namespace tp::obj {

	enum ListMethods {
		LISTOBJECT_PUSH_BACK,
		LISTOBJECT_GET_LENGTH,
	};

	struct ListObject : Object {
		static ObjectType TypeData;
		static void constructor(ObjectsContext* context, Object* self);
		static void copy(ObjectsContext* context, Object* self, const Object* in);
		static void destructor(ObjectsContext* context, Object* self);

		static alni allocated_size_recursive(ListObject* self);
		static alni allocated_size(ListObject* self);
		static Buffer<Object*> childs_retrival(ListObject* self);
		static void load(ObjectsContext* context, ArchiverIn& file_self, ListObject* self);
		static void save(ObjectsContext* context, ListObject* self, ArchiverOut& file_self);
		static alni save_size(ListObject* self);

		[[nodiscard]] const List<Object*>& getItems() const;

		void pushBack(Object* obj);
		void pushFront(Object* obj);
		void popBack(ObjectsContext* context);
		void delNode(ObjectsContext* context, List<Object*>::Node*);

	private:
		List<Object*> items;
	};
}