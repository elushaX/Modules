
#pragma once

#include "core/Object.hpp"

namespace tp::obj {

	extern struct NullObject* NdoNull_globalInstance;

	struct NullObject : Object {
		static ObjectType TypeData;

		static void destructor(Object* self);

		static void uninit();
		static Object* null_return() {
			if (!NdoNull_globalInstance) {
				NdoNull_globalInstance = objects_api::create<NullObject>();
				objects_api::increaseReferenceCount(NdoNull_globalInstance);
			}
			return (Object*) NdoNull_globalInstance;
		}

		static Object* null_return_ref() {
			objects_api::increaseReferenceCount(null_return());
			return NdoNull_globalInstance;
		}
	};

#define NDO_NULL NullObject::null_return()
#define NDO_NULL_REF NullObject::null_return_ref()

}