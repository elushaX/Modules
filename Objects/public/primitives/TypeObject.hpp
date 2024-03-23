
#pragma once

#include "core/Object.hpp"

namespace tp::obj {
	struct TypeObject : Object {
		static ObjectType TypeData;
		const ObjectType* mTypeRef;
		static TypeObject* create(ObjectsContext* context, const ObjectType* type);
	};
}