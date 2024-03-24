
#pragma once

#include "Common.hpp"
#include "Map.hpp"
#include "List.hpp"
#include "Buffer.hpp"
#include "LocalConnection.hpp"

#include "core/TypeGroups.hpp"
#include "core/TypeMethods.hpp"

#include "ObjectArchiver.hpp"
#include "SizeCounter.hpp"

#include <string>

#include "core/ObjectArchiver.hpp"

/* Steps to create custom Object:
define name of object
define base type
define struct members
implement construct, destruct and copy methods */

namespace tp::obj {

	extern ModuleManifest gModuleObjects;

	extern struct objects_api* NDO;

	struct Object {
		Object* up;
		Object* down;
		alni flags;
		alni refc;

		const struct ObjectType* type;
	};

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

	struct ObjectTypeAriphmetics {
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
		const char* name;
		const ObjectTypeConversions* convesions = nullptr;
		const ObjectTypeAriphmetics* ariphmetics = nullptr;
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

#define SAVE_LOAD_MAX_CALLBACK_SLOTS 100
	typedef void(pre_save_callback)(void* self, ArchiverOut&);
	typedef void(pre_load_callback)(void* self, ArchiverIn&);
	typedef void(post_save_callback)(void* self, ArchiverOut&);
	typedef void(post_load_callback)(void* self, ArchiverIn&);
	typedef alni(slcb_size_callback)(void* self, ArchiverOut&);

	struct save_load_callbacks {
		void* self;
		pre_save_callback* pre_save;
		slcb_size_callback* size;
		pre_load_callback* pre_load;
		post_save_callback* post_save;
		post_load_callback* post_load;
	};

	struct objects_api {

		Map<std::string, const ObjectType*> types;
		TypeGroups type_groups;
		Interpreter* interp = nullptr;

		objects_api();
		~objects_api();

		void define(ObjectType* type);
		Object* create(const std::string& name);
		static Object* copy(Object* self, const Object* in);
		static bool compare(Object* first, Object* second);
		static Object* instantiate(Object*);
		static void set(Object* self, alni val);
		static void set(Object* self, alnf val);
		static void set(Object* self, const std::string& val);

		static alni toInt(Object* self);
		static alnf toFloat(Object* self);
		static bool toBool(Object* self);
		static std::string toString(Object* self);

		void clear_object_flags();

		void destroy(Object* in) const;

		static inline void increaseReferenceCount(Object* in) { in->refc++; }
		static inline alni getReferenceCount(Object* in) { return in->refc; }

	private:
		static inline void setReferenceCount(Object* in, halni count) { in->refc = count; }

	public:
		save_load_callbacks* sl_callbacks[SAVE_LOAD_MAX_CALLBACK_SLOTS]{};
		alni sl_callbacks_load_idx = 0;

		void add_sl_callbacks(save_load_callbacks*);

		alni objsize_file(Object* self);
		alni objsize_file_recursive(Object* self);

		alni objsize_ram(Object* self);
		alni objsize_ram_recursive_util(Object* self, const ObjectType* type, bool different_object = true);
		alni objsize_ram_recursive(Object* self);

		bool save(Object*, const std::string& path, bool compressed = true);
		Object* load(const std::string& path);
		alni save(ArchiverOut&, Object*);
		Object* load(ArchiverIn&, alni file_adress);

		template <typename Type>
		static Type* cast(const Object* in) {
			const ObjectType* typeIter = in->type;
			while (typeIter) {
				if (typeIter == &Type::TypeData) {
					return (Type*) in;
				}
				typeIter = typeIter->base;
			}
			return nullptr;
		}
	};

	void logTypeData(const ObjectType* type);
	void assertNoLeaks();
	ualni getObjCount();

	template <typename Type>
	Type* create() {
		return (Type*) NDO->create(Type::TypeData.name);
	}
}