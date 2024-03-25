
#pragma once

#include "ObjectType.hpp"

namespace tp::obj {

	// TODO : reconsider static variables
	extern ModuleManifest gModuleObjects;
	extern struct ObjectsContext* gObjectsContext;

	// Base object
	struct Object {
		// TODO : used for saving, can be removed
		Object* up;
		Object* down;
		alni flags;

		// reference counting for memory management
		alni references;

		// type information
		const ObjectType* type;

		// additional inherited data
	};

	struct ObjectsContext {
		ObjectsContext();
		~ObjectsContext();

		Map<std::string, const ObjectType*> types;
		TypeGroups type_groups;
		Interpreter* interp = nullptr;

		struct NullObject* nullObject = nullptr;

		struct BoolObject* boolTrueObject = nullptr;
		struct BoolObject* boolFalseObject = nullptr;
	};

	struct objects_api {
		static void initialize();
		static void finalize();

		static void define(ObjectType* type);
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

		static void clear_object_flags();

		static void destroy(Object* in);

		static inline void increaseReferenceCount(Object* in) { in->references++; }
		static inline alni getReferenceCount(Object* in) { return in->references; }

		static inline void setReferenceCount(Object* in, halni count) { in->references = count; }

		static alni objsize_file(Object* self);
		static alni objsize_file_recursive(Object* self);

		static alni objsize_ram(Object* self);
		static alni objsize_ram_recursive_util(Object* self, const ObjectType* type, bool different_object = true);
		static alni objsize_ram_recursive(Object* self);

		static bool save(Object*, const std::string& path, bool compressed = true);
		static Object* load(const std::string& path);
		static alni save(ArchiverOut&, Object*);
		static Object* load(ArchiverIn&, alni file_adress);

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

		static Object* create(const ObjectType* type);

		template <typename tObjectType>
		static tObjectType* create() {
			return (tObjectType*) create(&tObjectType::TypeData);
		}

		static Object* createByName(const char* name) { return create(gObjectsContext->types.get(name)); }

		static void logTypeData(const ObjectType* type);
		static void assertNoLeaks();
		static ualni getObjCount();

		static void addTypeToGroup(ObjectType* type, InitialierList<const char*> path, alni cur_arg = 0);

		static bool isType(const char* name);
		static const ObjectType* getType(const char* name);

		static NullObject* getNull();
		static NullObject* getNullReferenced();
	};
}