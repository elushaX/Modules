
#include "interpreter/Interpreter.hpp"

#include "primitives/PrimitiveObjects.hpp"

#include "Timing.hpp"

using namespace tp;
using namespace obj;

inline uint1 read_byte(ByteCode* bytecode) {
	auto out = (uint1) bytecode->mInstructions[bytecode->mInstructionIdx];
	bytecode->mInstructionIdx++;
	return out;
}

// exeption for opcodes
// reads 2 bytes from instructions in order to load constant onto operands stack
uint2 loadConstDataIdx(ByteCode* bytecode) {
	uint2 out = 0;
	out |= ((uint2) read_byte(bytecode));
	out |= ((uint2) read_byte(bytecode) << 8);
	return out;
}

// exeption for opcodes
// reads 2 bytes from instructions in order to load constant onto operands stack
void skip_param(ByteCode* bytecode) { bytecode->mInstructionIdx += 2; }

uint2 param(ByteCode* bytecode) { return loadConstDataIdx(bytecode); }

void Interpreter::exec(
	obj::MethodObject* method, obj::ClassObject* self, obj::DictObject* globals, InitialierList<GlobalDef> globals2
) {
	if (!method->mBytecodeLink->mBytecode.mInstructions.size()) {
		return;
	}

	mCallStack.enter({ nullptr, method, 0 });
	mCallStack.mStack.last().mSelf = self;

	stepBytecodeIn();

	if (globals) {
		for (auto glb : globals->getItems()) {
			mScopeStack.addLocal(glb->val, glb->key);
		}
	}

	for (auto global_def : globals2) {
		mScopeStack.addLocal(global_def.obj, global_def.id);
	}
}

bool Interpreter::finished() const { return !mCallStack.len(); }

void Interpreter::stepBytecode() {
	halni call_depth = mCallStack.len();
	do {
		stepBytecodeIn();
		if (finished()) {
			return;
		}
	} while (call_depth != mCallStack.len());
}

void Interpreter::stepBytecodeOut() {
	if (finished()) {
		return;
	}

	halni call_depth = mCallStack.len();
	if (!call_depth) {
		return;
	}

	do {
		stepBytecodeIn();
		if (finished()) {
			return;
		}
	} while (call_depth - 1 != mCallStack.len());
}

void Interpreter::stepBytecodeIn() {
	using namespace obj;
	using namespace tp;

	DEBUG_BREAK(finished());

	auto bytecode = mCallStack.getBytecode();

	if (bytecode->mInstructionIdx >= (ualni) bytecode->mInstructions.size()) {
		// just return
		if (mScopeStack.mIdx) {
			mScopeStack.leaveScope();
		}

		mCallStack.leave();

		if (mCallStack.len()) {
			mOperandsStack.push(objects_api::getNullReferenced());
		}
		return;
	}

	auto opcode = bytecode->mInstructions[bytecode->mInstructionIdx];
	bytecode->mInstructionIdx++;

	switch (opcode) {

		case OpCode::NONE:
			{
				break;
			}

		case OpCode::HALT:
			{
				while (true) {
					sleep(3);
				}
				break;
			}

		case OpCode::TERMINATE:
			{
				// terminate(0);
			}

		case OpCode::IGNORE:
			{
				mOperandsStack.getOperand();
				break;
			}

		case OpCode::LOAD_CONST:
			{
				auto idx = loadConstDataIdx(bytecode);
				auto const_obj = bytecode->mConstants[idx];
				mOperandsStack.push(const_obj);
				break;
			}

		case OpCode::LOAD_LOCAL:
			{
				auto idx = loadConstDataIdx(bytecode);
				auto const_obj = bytecode->mConstants[idx];
				auto local_id = objects_api::cast<StringObject>(const_obj);
				ASSERT(local_id && "Invalid Object Type");
				auto local = mScopeStack.getLocal(local_id->val);
				mOperandsStack.push(local);
				break;
			}

		case OpCode::SCOPE_IN:
			{
				mScopeStack.enterScope(true);
				break;
			}

		case OpCode::JUMP:
			{
				bytecode->mInstructionIdx += param(bytecode);
				break;
			}

		case OpCode::JUMP_IF:
			{
				auto cond = mOperandsStack.getOperand();
				if (objects_api::toBool(cond)) {
					bytecode->mInstructionIdx += param(bytecode);
				} else {
					skip_param(bytecode);
				}
				break;
			}

		case OpCode::JUMP_IF_NOT:
			{
				auto cond = mOperandsStack.getOperand();
				if (!objects_api::toBool(cond)) {
					bytecode->mInstructionIdx += param(bytecode);
				} else {
					skip_param(bytecode);
				}
				break;
			}

		case OpCode::JUMP_R:
			{
				bytecode->mInstructionIdx -= param(bytecode);
				break;
			}

		case OpCode::JUMP_IF_R:
			{
				auto cond = mOperandsStack.getOperand();
				if (objects_api::toBool(cond)) {
					bytecode->mInstructionIdx -= param(bytecode);
				} else {
					skip_param(bytecode);
				}
				break;
			}

		case OpCode::JUMP_IF_NOT_R:
			{
				auto cond = mOperandsStack.getOperand();
				if (!objects_api::toBool(cond)) {
					bytecode->mInstructionIdx -= param(bytecode);
				} else {
					skip_param(bytecode);
				}
				break;
			}

		case OpCode::SCOPE_OUT:
			{
				mScopeStack.leaveScope();
				break;
			}

		case OpCode::PRINT:
			{
				auto obj = mOperandsStack.getOperand();
				if (obj->type->conversions && obj->type->conversions->to_string) {
					auto str = objects_api::toString(obj);
					printf("%s\n", str.c_str());
				} else {
					printf("Object with type '%s' has no string representation.\n", obj->type->name);
				}
				break;
			}

		case OpCode::OBJ_CREATE_LOCAL:
			{
				auto type = mOperandsStack.getOperand<StringObject>();
				auto id = mOperandsStack.getOperand<StringObject>();
				mScopeStack.addLocal(objects_api::createByName(type->val.c_str()), id->val);
				break;
			}

		case OpCode::DEF_LOCAL:
			{
				auto id = mOperandsStack.getOperand<StringObject>();
				auto obj = mOperandsStack.getOperand();
				mScopeStack.addLocal(obj, id->val);
				objects_api::increaseReferenceCount(obj);
				// mScopeStack.popTemp();
				break;
			}

		case OpCode::OBJ_CREATE:
			{
				auto type = mOperandsStack.getOperand<StringObject>();

				// basic types
				auto idx = objects_api::isType(type->val.c_str());
				if (idx) {
					Object* new_obj = objects_api::createByName(type->val.c_str());

					mOperandsStack.push(new_obj);
					mScopeStack.addTemp(new_obj);
					break;
				}

				// class creation
				auto local_class = mScopeStack.getLocal(type->val);

				auto method = objects_api::cast<MethodObject>(local_class);
				ASSERT(method);

				// class is a function - execute it as a constructor

				// PUSH_ARGS protocol
				uint2 len = 0;
				mOperandsStack.push((Object*) (alni) len);
				mOperandsStack.push(nullptr);

				// CALL protocol
				mScopeStack.enterScope(false);
				mCallStack.enter({ nullptr, method, 0 });
				break;
			}

		case OpCode::CLASS_CONSTRUCT:
			{
				auto class_obj = objects_api::create<ClassObject>();

				for (auto local : mScopeStack.getCurrentScope()->mLocals) {
					class_obj->addMember(local->val, local->key);
				}

				mOperandsStack.push(class_obj);
				mScopeStack.addTempReturn(class_obj);

				mScopeStack.leaveScope();
				mCallStack.leave();
				return;
			}

		case OpCode::RETURN:
			{
				// mScopeStack.addTempReturn(NDO_NULL_REF);
				mScopeStack.leaveScope();
				mCallStack.leave();
				if (mCallStack.len()) {
					mOperandsStack.push(objects_api::getNullReferenced());
				}
				break;
			}

		case OpCode::PUSH_ARGS:
			{

				// Layout of OperandsStack:
				// ....
				// +1) length : to chech number of args
				// +2) nullptr : stop saving args
				// +3) object1
				// +4) object2
				// ...

				uint2 len = read_byte(bytecode);
				mOperandsStack.push((Object*) (alni) len);
				mOperandsStack.push(nullptr);
				break;
			}

		case OpCode::SAVE_ARGS:
			{
				uint2 args_len = read_byte(bytecode);
				auto argument = mOperandsStack.getOperand();

				while (argument) {
					objects_api::increaseReferenceCount(argument);

					auto argument_id = bytecode->mConstants[loadConstDataIdx(bytecode)];
					auto id = objects_api::cast<StringObject>(argument_id);
					DEBUG_ASSERT(id);
					mScopeStack.addLocal(argument, id->val);
					bytecode->mArgumentsLoaded++;
					argument = mOperandsStack.getOperand();
				}

				uint2 saved_len = uint2(ualni(mOperandsStack.getOperand()));
				ASSERT(args_len == saved_len && "invalid number of arguments passefd");

				break;
			}

		case OpCode::RETURN_OBJ:
			{
				if (mCallStack.len() > 1) {
					auto ret = mOperandsStack.getOperand();
					mOperandsStack.push(ret);
					mScopeStack.addTempReturn(ret);
				}

				mScopeStack.leaveScope();
				mCallStack.leave();
				break;
			}

		case OpCode::CALL:
			{
				auto obj = mOperandsStack.getOperand();

				if (!mIsTypeMethod) {
					auto method = objects_api::cast<MethodObject>(obj);

					mScopeStack.enterScope(false);
					mCallStack.enter({ nullptr, method, 0 });

					// push self
					mCallStack.mStack.last().mSelf = mLastParent;
					break;
				}

				(*mTypeMethod)(this);
				mIsTypeMethod = false;
				mTypeMethod = nullptr;
				break;
			}

		case OpCode::OBJ_ADD:
			{
				auto left = mOperandsStack.getOperand();
				auto right = mOperandsStack.getOperand();

				ASSERT(left->type == right->type && "addition of different types is not implemented");
				ASSERT(left->type->arithmetics && left->type->arithmetics->add && "cannot add object of this type");

				auto res = objects_api::instantiate(left);
				res->type->arithmetics->add(res, right);

				mScopeStack.addTemp(res);
				mOperandsStack.push(res);
				break;
			}
		case OpCode::OBJ_SUB:
			{
				auto left = mOperandsStack.getOperand();
				auto right = mOperandsStack.getOperand();

				ASSERT(left->type == right->type && "addition of different types is not implemented");
				ASSERT(left->type->arithmetics && left->type->arithmetics->add && "cannot add object of this type");

				auto res = objects_api::instantiate(left);
				res->type->arithmetics->sub(res, right);

				mScopeStack.addTemp(res);
				mOperandsStack.push(res);
				break;
			}
		case OpCode::OBJ_MUL:
			{
				auto left = mOperandsStack.getOperand();
				auto right = mOperandsStack.getOperand();

				ASSERT(left->type == right->type && "addition of different types is not implemented");
				ASSERT(left->type->arithmetics && left->type->arithmetics->add && "cannot add object of this type");

				auto res = objects_api::instantiate(left);
				res->type->arithmetics->mul(res, right);

				mScopeStack.addTemp(res);
				mOperandsStack.push(res);
				break;
			}
		case OpCode::OBJ_DIV:
			{
				auto left = mOperandsStack.getOperand();
				auto right = mOperandsStack.getOperand();

				ASSERT(left->type == right->type && "addition of different types is not implemented");
				ASSERT(left->type->arithmetics && left->type->arithmetics->add && "cannot add object of this type");

				auto res = objects_api::instantiate(left);
				res->type->arithmetics->div(res, right);

				mScopeStack.addTemp(res);
				mOperandsStack.push(res);
				break;
			}

		case OpCode::OBJ_COPY:
			{
				auto left = mOperandsStack.getOperand();
				auto right = mOperandsStack.getOperand();
				objects_api::copy(left, right);
				break;
			}

		case OpCode::CHILD:
			{
				auto child_id = mOperandsStack.getOperand<StringObject>();
				auto parent = mOperandsStack.getOperand();
				bool is_method = read_byte(bytecode);

				Object* child = nullptr;
				TypeMethods::LookupKey tm_key;

				if (is_method) {
					tm_key = TypeMethods::presents(parent->type, child_id->val);
				}

				if (tm_key) {
					mTypeMethod = TypeMethods::getMethod(parent->type, tm_key);
					mIsTypeMethod = true;

				} else {
					auto class_obj = objects_api::cast<ClassObject>(parent);

					ASSERT(class_obj && "not a class object");
					auto idx = class_obj->members->presents(child_id->val);
					ASSERT(idx && "No child with such id");
					child = class_obj->members->getSlotVal(idx);
				}

				// mScopeStack.addTemp(obj);
				mOperandsStack.push(child);
				mLastParent = parent;
				break;
			}

		case OpCode::SELF:
			{
				// mScopeStack.addTemp(obj);
				ASSERT(mCallStack.mStack.last().mSelf);
				mOperandsStack.push(mCallStack.mStack.last().mSelf);
				break;
			}

		case OpCode::OBJ_LOAD:
			{
				auto path = mOperandsStack.getOperand<StringObject>();
				auto obj = objects_api::load(path->val);
				mScopeStack.addTemp(obj);
				mOperandsStack.push(obj);
				break;
			}
		case OpCode::OBJ_SAVE:
			{
				auto path = mOperandsStack.getOperand<StringObject>();
				auto target = mOperandsStack.getOperand();
				objects_api::save(target, path->val);
				break;
			}

		case OpCode::AND:
			{
				auto left = objects_api::toBool(mOperandsStack.getOperand());
				auto right = objects_api::toBool(mOperandsStack.getOperand());
				auto out = BoolObject::create(left && right);
				mScopeStack.addTemp(out);
				mOperandsStack.push(out);
				break;
			}
		case OpCode::OR:
			{
				auto left = objects_api::toBool(mOperandsStack.getOperand());
				auto right = objects_api::toBool(mOperandsStack.getOperand());
				auto out = BoolObject::create(left || right);
				mScopeStack.addTemp(out);
				mOperandsStack.push(out);
				break;
			}
		case OpCode::NOT:
			{
				auto inv = objects_api::toBool(mOperandsStack.getOperand());
				auto out = BoolObject::create(!inv);
				mScopeStack.addTemp(out);
				mOperandsStack.push(out);
				break;
			}
		case OpCode::NOT_EQUAL:
			{
				auto left = mOperandsStack.getOperand();
				auto right = mOperandsStack.getOperand();
				auto out = BoolObject::create(!objects_api::compare(left, right));
				mScopeStack.addTemp(out);
				mOperandsStack.push(out);
				break;
			}
		case OpCode::EQUAL:
			{
				auto left = mOperandsStack.getOperand();
				auto right = mOperandsStack.getOperand();
				auto out = BoolObject::create(objects_api::compare(left, right));
				mScopeStack.addTemp(out);
				mOperandsStack.push(out);
				break;
			}

		case OpCode::MORE:
			{
				auto left = objects_api::toFloat(mOperandsStack.getOperand());
				auto right = objects_api::toFloat(mOperandsStack.getOperand());
				auto out = BoolObject::create(left > right);
				mScopeStack.addTemp(out);
				mOperandsStack.push(out);
				break;
			}
		case OpCode::LESS:
			{
				auto left = objects_api::toFloat(mOperandsStack.getOperand());
				auto right = objects_api::toFloat(mOperandsStack.getOperand());
				auto out = BoolObject::create(left < right);
				mScopeStack.addTemp(out);
				mOperandsStack.push(out);
				break;
			}
		case OpCode::EQUAL_OR_MORE:
			{
				auto left = objects_api::toFloat(mOperandsStack.getOperand());
				auto right = objects_api::toFloat(mOperandsStack.getOperand());
				auto out = BoolObject::create(left >= right);
				mScopeStack.addTemp(out);
				mOperandsStack.push(out);
				break;
			}
		case OpCode::EQUAL_OR_LESS:
			{
				auto left = objects_api::toFloat(mOperandsStack.getOperand());
				auto right = objects_api::toFloat(mOperandsStack.getOperand());
				auto out = BoolObject::create(left <= right);
				mScopeStack.addTemp(out);
				mOperandsStack.push(out);
				break;
			}

		default:
			{
				ASSERT("Invalid OpCode");
			}
	}
}

void Interpreter::execAll(
	obj::MethodObject* method, obj::ClassObject* self, obj::DictObject* globals, InitialierList<GlobalDef> globals2
) {
	if (!method->mBytecodeLink->mBytecode.mInstructions.size()) {
		return;
	}

	exec(method, self, globals, globals2);
	while (!finished()) {
		stepBytecodeIn();
	}
}
