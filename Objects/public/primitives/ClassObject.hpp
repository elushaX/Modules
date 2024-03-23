#pragma once

#include "primitives/DictObject.hpp"

namespace obj {

	struct ClassObject : Object {

		static ObjectType TypeData;
		static void copy(ClassObject* self, const ClassObject* in);
		static void destructor(ClassObject* self);
		static void constructor(ClassObject* self);
		static tp::alni save_size(ClassObject* self);
		static void save(ClassObject* self, ArchiverOut& file_self);
		static void load(ArchiverIn& file_self, ClassObject* self);

		DictObject* members;

		void addMember(Object* obj, const std::string& id);
		void createMember(const std::string& type, const std::string& id);

		template<typename Type>
		Type* createMember(const std::string& id) {
			auto out = NDO->create(Type::TypeData.name);
			addMember(out, id);
			NDO->destroy(out);
			return (Type*) out;
		}

		template<typename Type>
		Type* getMember(const std::string& id) {
			auto idx = members->presents(id);
			if (bool(idx)) {
				return ((Type*)obj::ndo_cast(members->getSlotVal(idx), &Type::TypeData));
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
};