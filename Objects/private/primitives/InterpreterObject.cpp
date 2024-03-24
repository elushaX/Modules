
#include "primitives/InterpreterObject.hpp"
#include "primitives/LinkObject.hpp"
#include "primitives/MethodObject.hpp"

using namespace tp;
using namespace obj;

void InterpreterObject::constructor(InterpreterObject* self) {
	new (&self->mInterpreter) Interpreter();

	self->createMember("dict", "globals");
	self->createMember("link", "target method");
}

void InterpreterObject::destructor(InterpreterObject* self) { self->mInterpreter.~Interpreter(); }

void InterpreterObject::load(ArchiverIn& file_self, InterpreterObject* self) {
	new (&self->mInterpreter) Interpreter();
}

bool InterpreterObject::running() { return !mInterpreter.finished(); }

void InterpreterObject::exec(obj::ClassObject* self, tp::InitialierList<Interpreter::GlobalDef> globals) {

	if (running()) {
		return;
	}

	auto link_target = getMember<obj::LinkObject>("target method");
	if (!link_target) {
		return;
	}

	auto target = link_target->getLink();
	if (!target) {
		return;
	}

	auto method = objects_api::cast<MethodObject>(target);
	if (!method || !method->mScript->mBytecode.mInstructions.size()) {
		return;
	}

	mInterpreter.execAll(method, self, getMember<obj::DictObject>("globals"), globals);
}

void InterpreterObject::debug() {
	if (running()) {
		return;
	}

	auto link_target = getMember<obj::LinkObject>("target method");
	if (!link_target) {
		return;
	}

	auto target = link_target->getLink();
	if (!target) {
		return;
	}

	auto method = objects_api::cast<MethodObject>(target);
	if (!method || !method->mScript->mBytecode.mInstructions.size()) {
		return;
	}

	mInterpreter.exec(method, this, getMember<obj::DictObject>("globals"));
}

struct obj::ObjectType InterpreterObject::TypeData = {
	.base = &ClassObject::TypeData,
	.constructor = (object_constructor) InterpreterObject::constructor,
	.destructor = (object_destructor) InterpreterObject::destructor,
	.size = sizeof(InterpreterObject),
	.name = "interpreter",
	.load = (object_load) InterpreterObject::load,
};
