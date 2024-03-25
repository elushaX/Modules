
#include "primitives/BytecodeObject.hpp"

using namespace tp;
using namespace obj;

void BytecodeObject::constructor(BytecodeObject* self) {
	new (&self->mBytecode) ByteCode();
	self->mReadable = objects_api::create<StringObject>();
}

void BytecodeObject::copy(BytecodeObject* self, BytecodeObject* in) {
	// TODO : implement
	ASSERT(!"can not copy bytecode object")
}

void BytecodeObject::destructor(BytecodeObject* self) {
	self->mBytecode.~ByteCode();
	objects_api::destroy(self->mReadable);
}

tp::alni BytecodeObject::save_size(BytecodeObject* self) {
	alni out = sizeof(alni);                                 // string object
	out += sizeof(alni);                                     // constants size
	out += self->mBytecode.mConstants.size() * sizeof(alni); // constant objects
	out += SaveSizeCounter::calc(self->mBytecode.mInstructions);
	return out;
}

void BytecodeObject::save(BytecodeObject* self, ArchiverOut& file) {
	file << (alni) objects_api::save(file, self->mReadable);
	file << (alni) self->mBytecode.mConstants.size();

	for (auto const_obj : self->mBytecode.mConstants) {
		file << (alni) objects_api::save(file, const_obj.data());
	}

	// mInstructions
	file << self->mBytecode.mInstructions;
}

void BytecodeObject::load(ArchiverIn& file, obj::BytecodeObject* self) {
	new (&self->mBytecode) ByteCode();

	alni stringObjectAddress;
	file >> stringObjectAddress;
	self->mReadable = objects_api::cast<StringObject>(objects_api::load(file, stringObjectAddress));

	alni consts_count;
	file >> consts_count;

	self->mBytecode.mConstants.reserve(consts_count);

	for (auto const_obj : self->mBytecode.mConstants) {
		alni consts_addr;
		file >> consts_addr;
		const_obj.data() = objects_api::load(file, consts_addr);
	}

	file >> self->mBytecode.mInstructions;
}

struct ObjectType BytecodeObject::TypeData = {
	.base = nullptr,
	.constructor = (object_constructor) BytecodeObject::constructor,
	.destructor = (object_destructor) BytecodeObject::destructor,
	.copy = (object_copy) BytecodeObject::copy,
	.size = sizeof(BytecodeObject),
	.name = "bytecode",
	.conversions = nullptr,
	.save_size = (object_save_size) BytecodeObject::save_size,
	.save = (object_save) BytecodeObject::save,
	.load = (object_load) BytecodeObject::load,
};
