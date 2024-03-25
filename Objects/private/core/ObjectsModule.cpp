
#include "compiler/Functions.hpp"

#include "primitives/PrimitiveObjects.hpp"

using namespace tp;
using namespace obj;

static void defineTypes() {
	objects_api::define(&DictObject::TypeData);
	objects_api::define(&IntObject::TypeData);
	objects_api::define(&LinkObject::TypeData);
	objects_api::define(&ListObject::TypeData);
	objects_api::define(&NullObject::TypeData);
	objects_api::define(&StringObject::TypeData);
	objects_api::define(&BoolObject::TypeData);
	objects_api::define(&FloatObject::TypeData);
	objects_api::define(&EnumObject::TypeData);
	objects_api::define(&ClassObject::TypeData);
	objects_api::define(&ColorObject::TypeData);
	objects_api::define(&InterpreterObject::TypeData);
	objects_api::define(&TypeObject::TypeData);
}

static void defineGroups() {
	objects_api::addTypeToGroup(&DictObject::TypeData, { "Primitives" });
	objects_api::addTypeToGroup(&IntObject::TypeData, { "Primitives" });
	objects_api::addTypeToGroup(&LinkObject::TypeData, { "Primitives" });
	objects_api::addTypeToGroup(&ListObject::TypeData, { "Primitives" });
	objects_api::addTypeToGroup(&NullObject::TypeData, { "Primitives" });
	objects_api::addTypeToGroup(&StringObject::TypeData, { "Primitives" });
	objects_api::addTypeToGroup(&BoolObject::TypeData, { "Primitives" });
	objects_api::addTypeToGroup(&FloatObject::TypeData, { "Primitives" });
	objects_api::addTypeToGroup(&EnumObject::TypeData, { "Primitives" });
	objects_api::addTypeToGroup(&ClassObject::TypeData, { "Primitives" });
	objects_api::addTypeToGroup(&ColorObject::TypeData, { "Primitives" });
	objects_api::addTypeToGroup(&InterpreterObject::TypeData, { "scripting" });
}

static bool init(const ModuleManifest*) {
	objects_api::initialize();

	MethodObject::Initialize();

	defineTypes();
	defineGroups();

	return true;
}

static void deinit(const ModuleManifest*) {
	MethodObject::UnInitialize();

	objects_api::finalize();
}

static ModuleManifest* sModuleDependencies[] = { nullptr };

ModuleManifest obj::gModuleObjects = ModuleManifest("Objects", init, deinit, sModuleDependencies);
