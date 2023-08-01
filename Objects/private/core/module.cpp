
#include "NewPlacement.hpp"

#include "Tokenizer.hpp"

#include "compiler/function.h"

#include "primitives/boolobject.h"
#include "primitives/classobject.h"
#include "primitives/colorobject.h"
#include "primitives/dictobject.h"
#include "primitives/enumobject.h"
#include "primitives/floatobject.h"
#include "primitives/interpreterobject.h"
#include "primitives/intobject.h"
#include "primitives/linkobject.h"
#include "primitives/listobject.h"
#include "primitives/methodobject.h"
#include "primitives/nullobject.h"
#include "primitives/stringobject.h"
#include "primitives/typeobject.h"

using namespace obj;
using namespace tp;

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
	NDO->type_groups.addType(&DictObject::TypeData, {"Primitives"});
	NDO->type_groups.addType(&IntObject::TypeData, {"Primitives"});
	NDO->type_groups.addType(&LinkObject::TypeData, {"Primitives"});
	NDO->type_groups.addType(&ListObject::TypeData, {"Primitives"});
	NDO->type_groups.addType(&NullObject::TypeData, {"Primitives"});
	NDO->type_groups.addType(&StringObject::TypeData, {"Primitives"});
	NDO->type_groups.addType(&BoolObject::TypeData, {"Primitives"});
	NDO->type_groups.addType(&FloatObject::TypeData, {"Primitives"});
	NDO->type_groups.addType(&EnumObject::TypeData, {"Primitives"});
	NDO->type_groups.addType(&ClassObject::TypeData, {"Primitives"});
	NDO->type_groups.addType(&ColorObject::TypeData, {"Primitives"});
	NDO->type_groups.addType(&InterpreterObject::TypeData, { "scripting" });
}


static bool init(const tp::ModuleManifest*) {
	if (!NDO) NDO = new objects_api();
	obj::BCgen::init();

	MethodObject::Initialize();

	defineTypes();
	defineGroups();

	return true;
}

static void deinit(const tp::ModuleManifest*) {

	NullObject::uninit();
	MethodObject::UnInitialize();

	obj::BCgen::deinit();

	assertNoLeaks();

	delete NDO;
	NDO = nullptr;
}

static tp::ModuleManifest* sModuleDependencies[] = {
	// &tp::gModuleCompressor,
	&tp::gModuleMath, 
	&tp::gModuleStrings,
	&tp::gModuleTokenizer,
	&tp::gModuleConnection,
	NULL
};

tp::ModuleManifest obj::gModuleObjects = tp::ModuleManifest("Objects", init, deinit, sModuleDependencies);