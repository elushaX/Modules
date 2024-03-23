
#include "primitives/PrimitiveObjects.hpp"

using namespace tp;
using namespace obj;

static void defineTypes(ObjectsContext* context) {
	context->define(&DictObject::TypeData);
	context->define(&IntObject::TypeData);
	context->define(&LinkObject::TypeData);
	context->define(&ListObject::TypeData);
	context->define(&NullObject::TypeData);
	context->define(&StringObject::TypeData);
	context->define(&BoolObject::TypeData);
	context->define(&FloatObject::TypeData);
	context->define(&EnumObject::TypeData);
	context->define(&ClassObject::TypeData);
	context->define(&ColorObject::TypeData);
	context->define(&InterpreterObject::TypeData);
	context->define(&TypeObject::TypeData);
}

static void defineGroups(ObjectsContext* context) {
	context->type_groups.addType(&DictObject::TypeData, { "Primitives" });
	context->type_groups.addType(&IntObject::TypeData, { "Primitives" });
	context->type_groups.addType(&LinkObject::TypeData, { "Primitives" });
	context->type_groups.addType(&ListObject::TypeData, { "Primitives" });
	context->type_groups.addType(&NullObject::TypeData, { "Primitives" });
	context->type_groups.addType(&StringObject::TypeData, { "Primitives" });
	context->type_groups.addType(&BoolObject::TypeData, { "Primitives" });
	context->type_groups.addType(&FloatObject::TypeData, { "Primitives" });
	context->type_groups.addType(&EnumObject::TypeData, { "Primitives" });
	context->type_groups.addType(&ClassObject::TypeData, { "Primitives" });
	context->type_groups.addType(&ColorObject::TypeData, { "Primitives" });
	context->type_groups.addType(&InterpreterObject::TypeData, { "scripting" });
}

void obj::definePrimitiveTypes(ObjectsContext* context) {
	defineTypes(context);
	defineGroups(context);
}

static bool init(const ModuleManifest*) {

	NullObject::init();
	MethodObject::Initialize();

	return true;
}

static void deinit(const ModuleManifest*) {

	NullObject::uninit();
	MethodObject::UnInitialize();

	assertNoLeaks();
}