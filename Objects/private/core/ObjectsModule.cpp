
#include "compiler/Functions.hpp"

#include "primitives/BoolObject.hpp"
#include "primitives/ClassObject.hpp"
#include "primitives/ColorObject.hpp"
#include "primitives/DictObject.hpp"
#include "primitives/EnumObject.hpp"
#include "primitives/FloatObject.hpp"
#include "primitives/InterpreterObject.hpp"
#include "primitives/IntObject.hpp"
#include "primitives/LinkObject.hpp"
#include "primitives/ListObject.hpp"
#include "primitives/MethodObject.hpp"
#include "primitives/NullObject.hpp"
#include "primitives/StringObject.hpp"
#include "primitives/TypeObject.hpp"

using namespace tp;
using namespace obj;

static void defineTypes() {
	NDO->define(&DictObject::TypeData);
	NDO->define(&IntObject::TypeData);
	NDO->define(&LinkObject::TypeData);
	NDO->define(&ListObject::TypeData);
	NDO->define(&NullObject::TypeData);
	NDO->define(&StringObject::TypeData);
	NDO->define(&BoolObject::TypeData);
	NDO->define(&FloatObject::TypeData);
	NDO->define(&EnumObject::TypeData);
	NDO->define(&ClassObject::TypeData);
	NDO->define(&ColorObject::TypeData);
	NDO->define(&InterpreterObject::TypeData);
	NDO->define(&TypeObject::TypeData);
}

static void defineGroups() {
	NDO->type_groups.addType(&DictObject::TypeData, { "Primitives" });
	NDO->type_groups.addType(&IntObject::TypeData, { "Primitives" });
	NDO->type_groups.addType(&LinkObject::TypeData, { "Primitives" });
	NDO->type_groups.addType(&ListObject::TypeData, { "Primitives" });
	NDO->type_groups.addType(&NullObject::TypeData, { "Primitives" });
	NDO->type_groups.addType(&StringObject::TypeData, { "Primitives" });
	NDO->type_groups.addType(&BoolObject::TypeData, { "Primitives" });
	NDO->type_groups.addType(&FloatObject::TypeData, { "Primitives" });
	NDO->type_groups.addType(&EnumObject::TypeData, { "Primitives" });
	NDO->type_groups.addType(&ClassObject::TypeData, { "Primitives" });
	NDO->type_groups.addType(&ColorObject::TypeData, { "Primitives" });
	NDO->type_groups.addType(&InterpreterObject::TypeData, { "scripting" });
}

static bool init(const ModuleManifest*) {
	if (!NDO) NDO = new objects_api();
	obj::initialize();

	MethodObject::Initialize();

	defineTypes();
	defineGroups();

	return true;
}

static void deinit(const ModuleManifest*) {

	NullObject::uninit();
	MethodObject::UnInitialize();

	obj::finalize();

	assertNoLeaks();

	delete NDO;
	NDO = nullptr;
}

static ModuleManifest* sModuleDependencies[] = {
	nullptr
};

ModuleManifest obj::gModuleObjects = ModuleManifest("Objects", init, deinit, sModuleDependencies);
