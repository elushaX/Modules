
#pragma once

#include "core/Object.hpp"

namespace tp::obj {

	extern struct NullObject* NdoNull_globalInstance;

	struct NullObject : Object {

		static void destructor(Object* self);
		static ObjectType TypeData;
		static void uninit();
		static Object* null_return() {
			if (!NdoNull_globalInstance) {
				NdoNull_globalInstance = (NullObject*) NDO->create("null");
				NDO->increaseReferenceCount(NdoNull_globalInstance);
			}
			return (Object*) NdoNull_globalInstance;
		}

		static Object* null_return_ref() {
			NDO->increaseReferenceCount(null_return());
			return NdoNull_globalInstance;
		}
	};

	#define NDO_NULL NullObject::null_return()
	#define NDO_NULL_REF NullObject::null_return_ref()

}