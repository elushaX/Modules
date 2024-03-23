
#pragma once

#include "core/Object.hpp"

namespace obj {

	struct TypeObject : Object {
		static ObjectType TypeData;
		const ObjectType* mTypeRef;
		static TypeObject* create(const ObjectType* type);
	};
};