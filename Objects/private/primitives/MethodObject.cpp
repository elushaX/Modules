
#include "primitives/MethodObject.hpp"
#include "compiler/Functions.hpp"

using namespace tp;
using namespace obj;

void MethodObject::constructor(MethodObject* self) {
	// create empty bytecode
	self->mBytecodeLink = objects_api::create<BytecodeObject>();
}

void MethodObject::copy(MethodObject* self, MethodObject* in) {
	objects_api::destroy(self->mBytecodeLink);
	objects_api::increaseReferenceCount(in->mBytecodeLink);
	self->mBytecodeLink = in->mBytecodeLink;
}

void MethodObject::destructor(MethodObject* self) {
	// deference
	objects_api::destroy(self->mBytecodeLink);
}

tp::alni MethodObject::save_size(MethodObject* self) {
	// just reference to the bytecode
	return sizeof(Object*);
}

void MethodObject::save(MethodObject* self, ArchiverOut& file_self) {
	// save bytecode
	file_self << objects_api::save(file_self, self->mBytecodeLink);
}

void MethodObject::load(ArchiverIn& file_self, obj::MethodObject* self) {
	alni bytecodeAddress;
	file_self >> bytecodeAddress;
	auto bytecode = objects_api::load(file_self, bytecodeAddress);
	self->mBytecodeLink = objects_api::cast<BytecodeObject>(bytecode);
}

void MethodObject::compile() {
	// call to compiler
	Compile(this);
}

MethodObject* MethodObject::create(const std::string& script) {
	auto out = objects_api::create<MethodObject>();

	ASSERT(false)

	out->compile();
	return out;
}

struct ObjectType MethodObject::TypeData = {
	.base = nullptr,
	.constructor = (object_constructor) MethodObject::constructor,
	.destructor = (object_destructor) MethodObject::destructor,
	.copy = (object_copy) MethodObject::copy,
	.size = sizeof(MethodObject),
	.name = "method",
	.conversions = nullptr,
	.save_size = (object_save_size) MethodObject::save_size,
	.save = (object_save) MethodObject::save,
	.load = (object_load) MethodObject::load,
};
