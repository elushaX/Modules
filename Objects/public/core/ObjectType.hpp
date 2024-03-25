#pragma once

#include "ObjectArchiver.hpp"
#include "TypeMethods.hpp"
#include "TypeGroups.hpp"

namespace tp::obj {
	struct Object;

	typedef void (*object_from_int)(Object* self, alni in);
	typedef void (*object_from_float)(Object* self, alnf in);
	typedef void (*object_from_string)(Object* self, const std::string& in);
	typedef std::string (*object_to_string)(Object* self);
	typedef alni (*object_to_int)(Object* self);
	typedef alnf (*object_to_float)(Object* self);

	struct ObjectTypeConversions {
		object_from_int from_int;
		object_from_float from_float;
		object_from_string from_string;
		object_to_string to_string;
		object_to_int to_int;
		object_to_float to_float;
	};

	typedef void (*object_add)(Object* self, Object* operand);
	typedef void (*object_sub)(Object* self, Object* operand);
	typedef void (*object_mul)(Object* self, Object* operand);
	typedef void (*object_div)(Object* self, Object* operand);

	struct ObjectTypeArithmetics {
		object_add add;
		object_sub sub;
		object_mul mul;
		object_div div;
	};

	typedef void (*object_constructor)(Object* self);
	typedef void (*object_destructor)(Object* self);
	typedef void (*object_copy)(Object* self, const Object* target);

	typedef alni (*object_save_size)(Object* self);
	typedef void (*object_save)(Object*, ArchiverOut&);
	typedef void (*object_load)(ArchiverIn&, Object*);

	typedef bool (*object_compare)(Object*, Object*);

	typedef Buffer<Object*> (*object_debug_all_childs_retrival)(Object*);
	typedef alni (*object_allocated_size)(Object*);           // default value = type->size - sizeof(ObjectType*)
	typedef alni (*object_allocated_size_recursive)(Object*); // default value = object_allocated_size

	struct ObjectType {
		const ObjectType* base = nullptr;
		object_constructor constructor = nullptr;
		object_destructor destructor = nullptr;
		object_copy copy = nullptr;
		alni size = 0;
		const char* name = nullptr;
		const ObjectTypeConversions* conversions = nullptr;
		const ObjectTypeArithmetics* arithmetics = nullptr;
		object_save_size save_size = nullptr;
		object_save save = nullptr;
		object_load load = nullptr;
		object_compare comparison = nullptr;
		void* vtable = nullptr;
		const char* description = nullptr;
		object_debug_all_childs_retrival childs_retrival = nullptr;
		object_allocated_size allocated_size = nullptr;
		object_allocated_size_recursive allocated_size_recursive = nullptr;
		TypeMethods type_methods;
	};
}