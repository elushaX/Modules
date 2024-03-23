
#pragma once

#include "Common.hpp"
#include "Map.hpp"
#include "List.hpp"
#include "Buffer.hpp"
#include "LocalConnection.hpp"

#include "core/TypeGroups.hpp"
#include "core/TypeMethods.hpp"

#include "Archiver.hpp"
#include "SizeCounter.hpp"

#include <string>

//#include "interpreter/interpreter.h"

/* Steps to create custom Object:
define name of object
define base type
define struct members
implement construct, destruct and copy methods */


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


	template<bool tRead>
	class Archiver : public tp::ArchiverTemplate<tRead> {
		tp::LocalConnection mConnection;
		tp::ualni mAddress = 0;
		tp::ualni mFirstNotWritten = 0;

	public:
		Archiver() = default;
		explicit Archiver(const char* location) {
				mConnection.connect(tp::LocalConnection::Location(location), tp::LocalConnection::Type(tRead));
		};

		void writeBytes(const tp::int1* val, tp::ualni size) override {
			mConnection.setPointer(mAddress);
			mConnection.writeBytes(val, size);
			incrementAddresses(size);
		}

		void readBytes(tp::int1* val, tp::ualni size) override {
			mConnection.setPointer(mAddress);
			mConnection.readBytes(val, size);
			incrementAddresses(size);
		}

		tp::ualni getAddress() { return mAddress; }

		void setAddress(tp::ualni addr) { mAddress = addr; }

		tp::ualni getFreeAddress() { return mFirstNotWritten; }

		void setFreeAddress(tp::ualni addr) { mFirstNotWritten = addr; }

		bool isOpened() { return mConnection.getConnectionStatus().isOpened(); }

		tp::ualni getSize() { return mConnection.size(); }

	private:
		void incrementAddresses(tp::ualni size) {
			// if (mAddress + size > mFirstNotWritten) mFirstNotWritten = mAddress + size;
			mAddress += size;
		}
	};

	using ArchiverIn = Archiver<true>;
	using ArchiverOut = Archiver<false>;

	void save_string(ArchiverOut& file, const std::string& string);
	tp::ualni save_string_size(const std::string& string);
	void load_string(ArchiverIn& file, std::string& out);

	extern tp::ModuleManifest gModuleObjects;

	extern struct objects_api* NDO;

	struct ObjectMemHead {
		ObjectMemHead* up;
		ObjectMemHead* down;
		tp::alni flags;
		tp::alni refc;
	};

	struct Object {
		const struct ObjectType* type;
	};

	typedef void (*object_from_int)(Object* self, tp::alni in);
	typedef void (*object_from_float)(Object* self, tp::alnf in);
	typedef void (*object_from_string)(Object* self, const std::string& in);
	typedef std::string(*object_to_string)(Object* self);
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
	typedef void (*object_save)(Object*, ArchiverOut&);
	typedef void (*object_load)(ArchiverIn&, Object*);

	typedef bool (*object_compare)(Object*, Object*);

	typedef tp::Buffer<Object*> (*object_debug_all_childs_retrival)(Object*);
	typedef tp::alni (*object_allocated_size)(Object*); // default value = type->size - sizeof(ObjectType*)
	typedef tp::alni (*object_allocated_size_recursive)(Object*); // default value = object_allocated_size

	struct ObjectType {
		const ObjectType* base = nullptr;
		object_constructor constructor = nullptr;
		object_destructor destructor = nullptr;
		object_copy copy = nullptr;
		tp::alni size = 0;
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
	typedef void (pre_save_callback)(void* self, ArchiverOut&);
	typedef void (pre_load_callback)(void* self, ArchiverIn&);
	typedef void (post_save_callback)(void* self, ArchiverOut&);
	typedef void (post_load_callback)(void* self, ArchiverIn&);
	typedef tp::alni (slcb_size_callback)(void* self, ArchiverOut&);

	struct save_load_callbacks {
		void* self;
		pre_save_callback* pre_save;
		slcb_size_callback* size;
		pre_load_callback* pre_load;
		post_save_callback* post_save;
		post_load_callback* post_load;
	};


	struct objects_api {

		tp::Map<std::string, const ObjectType*> types;
		obj::TypeGroups type_groups;
		Interpreter* interp = nullptr;

		objects_api();
		~objects_api();

		void define(ObjectType* type);
		Object* create(const std::string& name);
		Object* copy(Object* self, const Object* in);
		bool compare(Object* first, Object* second);
		Object* instatiate(Object*);
		void set(Object* self, tp::alni val);
		void set(Object* self, tp::alnf val);
		void set(Object* self, const std::string& val);

		tp::alni toInt(Object* self);
		tp::alnf toFloat(Object* self);
		bool toBool(Object* self);
		std::string toString(Object* self);

		void clear_object_flags();

		void destroy(Object* in);

		void refinc(Object* in);
		tp::halni getrefc(Object* in);
	private:
		void setrefc(Object* in, tp::halni refc);
	public:

		save_load_callbacks* sl_callbacks[SAVE_LOAD_MAX_CALLBACK_SLOTS];
		tp::alni sl_callbacks_load_idx = 0;

		void add_sl_callbacks(save_load_callbacks*);

		tp::alni objsize_file(Object* self);
		tp::alni objsize_file_recursive(Object* self);

		tp::alni objsize_ram(Object* self);
		tp::alni objsize_ram_recursive_util(Object* self, const ObjectType* type, bool different_object = true);
		tp::alni objsize_ram_recursive(Object* self);

		bool save(Object*, const std::string& path, bool compressed = true);
		Object* load(const std::string& path);
		tp::alni save(ArchiverOut&, Object*);
		Object* load(ArchiverIn&, tp::alni file_adress);
	};

	void logTypeData(const ObjectType* type);
	void assertNoLeaks();
	tp::ualni getObjCount();

	Object* ndo_cast(const Object* in, const ObjectType* to_type);

	template <typename Type>
	Type* create() {
		return (Type*)obj::NDO->create(Type::TypeData.name);
	}
};