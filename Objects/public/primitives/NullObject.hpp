
#pragma once

#include "core/Object.hpp"

namespace tp::obj {

	struct NullObject : Object {
		static ObjectType TypeData;
	};

#define NDO_NULL objects_api::getNull()
#define NDO_NULL_REF objects_api::getNullReferenced()

}