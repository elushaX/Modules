#pragma once

#include "primitives/DictObject.hpp"

namespace tp::obj {

	struct ClassObject : Object {

		static ObjectType TypeData;
		static void copy(ObjectsContext* context, ClassObject* self, const ClassObject* in);
		static void destructor(ObjectsContext* context, ClassObject* self);
		static void constructor(ObjectsContext* context, ClassObject* self);
		static alni save_size(ClassObject* self);
		static void save(ObjectsContext* context, ClassObject* self, ArchiverOut& file_self);
		static void load(ObjectsContext* context, ArchiverIn& file_self, ClassObject* self);

		DictObject* members;

		void addMember(Object* obj, const std::string& id);
		void createMember(ObjectsContext* context, const std::string& type, const std::string& id);

		template<typename Type>
		Type* createMember(ObjectsContext* context, const std::string& id) {
			auto out = context->create(Type::TypeData.name);
			addMember(out, id);
			context->destroy(out);
			return (Type*) out;
		}

		template<typename Type>
		Type* getMember(const std::string& id) {
			auto idx = members->presents(id);
			if (bool(idx)) {
				return ((Type*)castObject(members->getSlotVal(idx), &Type::TypeData));
			}
			return NULL;
		}

		template<typename Type>
		Type* getMemberAssert(const std::string& id) {
			auto out = getMember<Type>(id);
			assert(out && "invalid member access");
			return out;
		}
	};
}