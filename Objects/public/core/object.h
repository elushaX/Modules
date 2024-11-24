
#pragma once

#include "Common.hpp"
#include "Map.hpp"
#include "List.hpp"
#include "Buffer.hpp"
#include "Strings.hpp"
#include "LocalConnection.hpp"

#include "core/typegroups.h"
#include "core/typemethods.h"

//#include "interpreter/interpreter.h"

/* Steps to create custom Object:
define name of object
define base type
define struct members
implement construct, destruct and copy methods */

#define OBJECT_REF_COUNT

#ifdef _DEBUG
#define NDO_CAST(cast_type, ptr) ((cast_type*)ndo_cast(ptr, &cast_type::TypeData))
#define NDO_CAST_ASSERT(cast_type, ptr) {assert(ndo_cast(ptr, &cast_type::TypeData))}
#else
#define NDO_CAST_ASSERT(cast_type, ptr) {assert(ndo_cast(ptr, &cast_type::TypeData))}
#define NDO_CAST(cast_type, ptr) ((cast_type*)ndo_cast(ptr, &cast_type::TypeData))
#endif

#define NDO_CASTV(cast_type, ptr, var_name) cast_type* var_name = NDO_CAST(cast_type, ptr); var_name

#define NDO_MEMH_FROM_NDO(ndo_ptr) (((ObjectMemHead*)ndo_ptr) - 1)
#define NDO_FROM_MEMH(ndo_ptr) ((Object*)(ndo_ptr + 1))

namespace obj {

	class Archiver : public tp::LocalConnection {
	public:
		enum Type { SAVE, LOAD };

		Archiver() = default;
		Archiver(const char*, Type) {};

		bool opened;

		tp::ualni adress{};

		// not yet writen address
		// always offsets on write without specific address
		tp::ualni avl_adress{};

		void write_bytes(const tp::int1* in, tp::alni size, tp::alni adress = -1) {}

		template <typename Type>
		void write(Type* in, tp::alni adress = -1) {
			write_bytes((tp::int1*) in, sizeof(Type), adress);
		}

		void read_bytes(tp::int1* in, tp::alni size, tp::alni adress = -1) {}

		template <typename Type>
		void read(Type* in, tp::alni adress = -1) {
			read_bytes((tp::int1*) in, sizeof(Type), adress);
		}
	};

	extern tp::ModuleManifest gModuleObjects;

	extern struct objects_api* NDO;

	struct ObjectMemHead {
		ObjectMemHead* up;
		ObjectMemHead* down;
		tp::alni flags;
		#ifdef OBJECT_REF_COUNT
		tp::alni refc;
		#endif
	};

	struct Object {
		const struct ObjectType* type;
	};

	typedef void (*object_from_int)(Object* self, tp::alni in);
	typedef void (*object_from_float)(Object* self, tp::alnf in);
	typedef void (*object_from_string)(Object* self, tp::String in);
	typedef tp::String(*object_to_string)(Object* self);
	typedef tp::alni(*object_to_int)(Object* self);
	typedef tp::alnf(*object_to_float)(Object* self);

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

	typedef tp::alni(*object_save_size)(Object* self);
	typedef void (*object_save)(Object*, Archiver&);
	typedef void (*object_load)(Archiver&, Object*);

	typedef bool (*object_compare)(Object*, Object*);

	typedef tp::Buffer<Object*> (*object_debug_all_childs_retrival)(Object*);
	typedef tp::alni (*object_allocated_size)(Object*); // default value = type->size - sizeof(ObjectType*)
	typedef tp::alni (*object_allocated_size_recursive)(Object*); // default value = object_allocated_size

	struct ObjectType {
		const ObjectType* base = NULL;
		object_constructor constructor = NULL;
		object_destructor destructor = NULL;
		object_copy copy = NULL;
		tp::alni size = NULL;
		const char* name;
		const ObjectTypeConversions* convesions = NULL;
		const ObjectTypeAriphmetics* ariphmetics = NULL;
		object_save_size save_size = NULL;
		object_save save = NULL;
		object_load load = NULL;
		object_compare comparison = NULL;
		void* vtable = NULL;
		const char* description = NULL;
		object_debug_all_childs_retrival childs_retrival = NULL;
		object_allocated_size allocated_size = NULL;
		object_allocated_size_recursive allocated_size_recursive = NULL;
		TypeMethods type_methods;
	};


	#define SAVE_LOAD_MAX_CALLBACK_SLOTS 100
	typedef void (pre_save_callback)(void* self, Archiver&);
	typedef void (pre_load_callback)(void* self, Archiver&);
	typedef void (post_save_callback)(void* self, Archiver&);
	typedef void (post_load_callback)(void* self, Archiver&);
	typedef tp::alni (slcb_size_callback)(void* self, Archiver&);

	struct save_load_callbacks {
		void* self;
		pre_save_callback* pre_save;
		slcb_size_callback* size;
		pre_load_callback* pre_load;
		post_save_callback* post_save;
		post_load_callback* post_load;
	};


	struct objects_api {

		tp::Map<tp::String, const ObjectType*> types;
		obj::TypeGroups type_groups;
		Interpreter* interp = NULL;

		objects_api();
		~objects_api();

		void define(ObjectType* type);
		Object* create(const tp::String& name);
		Object* copy(Object* self, const Object* in);
		bool compare(Object* first, Object* second);
		Object* instatiate(Object*);
		void set(Object* self, tp::alni val);
		void set(Object* self, tp::alnf val);
		void set(Object* self, tp::String val);

		tp::alni toInt(Object* self);
		tp::alnf toFloat(Object* self);
		bool toBool(Object* self);
		tp::String toString(Object* self);

		void clear_object_flags();

		void destroy(Object* in);

		#ifdef OBJECT_REF_COUNT
		void refinc(Object* in);
		tp::halni getrefc(Object* in);
	private:
		void setrefc(Object* in, tp::halni refc);
	public:
		#endif

		save_load_callbacks* sl_callbacks[SAVE_LOAD_MAX_CALLBACK_SLOTS];
		tp::alni sl_callbacks_load_idx = 0;

		void add_sl_callbacks(save_load_callbacks*);

		tp::alni objsize_file(Object* self);
		tp::alni objsize_file_recursive(Object* self);

		tp::alni objsize_ram(Object* self);
		tp::alni objsize_ram_recursive_util(Object* self, const ObjectType* type, bool different_object = true);
		tp::alni objsize_ram_recursive(Object* self);

		bool save(Object*, tp::String path, bool compressed = true);
		Object* load(tp::String path);
		tp::alni save(Archiver&, Object*);
		Object* load(Archiver&, tp::alni file_adress);
	};

	Object* ndo_cast(const Object* in, const ObjectType* to_type);

	template <typename Type>
	Type* create() {
		return (Type*)obj::NDO->create(Type::TypeData.name);
	}
};