
#include "compiler/Functions.hpp"

#include "primitives/MethodObject.hpp"
#include "primitives/PrimitiveObjects.hpp"

#include "parser/Parser.hpp"

using namespace tp;
using namespace obj;

void obj::Generate(ByteCode& out, StatementScope* body) {
	auto root = FunctionDefinition();

	root.inst(Instruction(OpCode::SCOPE_IN));

	for (auto child_stm : body->mStatements) {
		root.EvalStatement(child_stm.data());
	}

	root.inst(Instruction(OpCode::SCOPE_OUT));

	root.generateByteCode(out);
}

ConstObject* FunctionDefinition::defineLocal(const std::string& id) {
	// auto idx = mLocals.presents(id);
	// RelAssert(!idx && "Local Redefinition");
	auto const_str_id = mConstants.get(id);
	mLocals.put(id, const_str_id);
	return const_str_id;
}

FunctionDefinition::
	FunctionDefinition(const std::string& function_id, const Buffer<std::string>& args, FunctionDefinition*) {
	mFunctionId = function_id;
	inst(Instruction(OpCode::SAVE_ARGS, (alni) args.size(), 1));
	for (auto id : args) {
		ASSERT(!mLocals.presents(id.data()) && "Argument Redefinition")
		auto const_data = mConstants.get(id.data());
		mArgsOrder.pushBack(const_data);
		mLocals.put(id.data(), const_data);
		inst(Instruction(const_data));
	}
}

void FunctionDefinition::EvalStatement(Statement* stm) {
	switch (stm->mType) {
		case Statement::Type::IGNORE:
			{
				auto stm_ignore = (StatementIgnore*) stm;
				EvalExpr(stm_ignore->mExpr);
				inst(OpCode::IGNORE);
				break;
			}
		case Statement::Type::WHILE:
			{
				auto stm_while = (StatementWhile*) stm;

				auto check_mark = inst(Instruction());

				EvalExpr(stm_while->mCondition);

				auto jump_if_inst = inst(Instruction(nullptr, Instruction::InstType::JUMP_IF_NOT));

				if (stm_while->mScope) {
					EvalStatement(stm_while->mScope);
				}

				auto jump_inst = inst(Instruction(nullptr, Instruction::InstType::JUMP));
				auto end_mark = inst(Instruction());

				jump_if_inst->data.mInstTarget = &end_mark->data;
				jump_inst->data.mInstTarget = &check_mark->data;

				break;
			}
		case Statement::Type::IF:
			{
				auto stm_if = (StatementIf*) stm;

				EvalExpr(stm_if->mCondition);

				auto jump_if_inst = inst(Instruction(nullptr, Instruction::InstType::JUMP_IF_NOT));

				if (stm_if->mOnTrue) {
					EvalStatement(stm_if->mOnTrue);
				}

				auto jump_inst = inst(Instruction(nullptr, Instruction::InstType::JUMP));
				auto else_mark = inst(Instruction());

				if (stm_if->mOnFalse) {
					EvalStatement(stm_if->mOnFalse);
				}

				auto end_mark = inst(Instruction());

				jump_if_inst->data.mInstTarget = &else_mark->data;
				jump_inst->data.mInstTarget = &end_mark->data;

				break;
			}
		case Statement::Type::SCOPE:
			{
				auto stm_scope = (StatementScope*) stm;
				if (stm_scope->mPushToScopeStack) {
					inst(Instruction(OpCode::SCOPE_IN));
				}
				for (auto child_stm : stm_scope->mStatements) {
					EvalStatement(child_stm.data());
				}
				if (stm_scope->mPushToScopeStack) {
					inst(Instruction(OpCode::SCOPE_OUT));
				}
				break;
			}
		case Statement::Type::DEF_FUNC:
			{
				auto stm_func_def = (StatementFuncDef*) stm;

				FunctionDefinition func(stm_func_def->mFunctionId, stm_func_def->mArgs, this);

				// define method as local
				auto idx = mLocals.presents(func.mFunctionId);
				ASSERT(!idx && "Local Redefinition with function name")
				mLocals.put(func.mFunctionId, mConstants.get(func.mFunctionId));

				// create and register const func object
				auto function_obj = objects_api::create<MethodObject>();
				auto method_const_obj = mConstants.addMethod(func.mFunctionId, function_obj);

				for (auto child_stm : stm_func_def->mStatements->mStatements) {
					func.EvalStatement(child_stm.data());
				}

				func.generateByteCode(function_obj->mBytecodeLink->mBytecode);

				inst(Instruction(OpCode::LOAD_CONST, method_const_obj));
				inst(Instruction(OpCode::LOAD_CONST, mConstants.get(func.mFunctionId)));
				inst(Instruction(OpCode::DEF_LOCAL));
				break;
			}
		case Statement::Type::CLASS_DEF:
			{

				// do the function definition
				auto stm_class_def = (StatementClassDef*) stm;

				FunctionDefinition func(stm_class_def->mClassId, {}, this);

				// define method as local
				auto idx = mLocals.presents(func.mFunctionId);
				ASSERT(!idx && "Local Redefinition with function name")
				mLocals.put(func.mFunctionId, mConstants.get(func.mFunctionId));

				// create and register const func object
				auto function_obj = objects_api::create<MethodObject>();
				auto method_const_obj = mConstants.addMethod(func.mFunctionId, function_obj);

				// compile function
				for (auto child_stm : stm_class_def->mScope->mStatements) {
					// check for return statements
					ASSERT(
						child_stm.data()->mType != Statement::Type::RET && "return statements are not allowed in class definition"
					)
					func.EvalStatement(child_stm.data());
				}
				// create one last instruction - constructing class from function execution state
				func.inst(Instruction(OpCode::CLASS_CONSTRUCT));
				func.generateByteCode(function_obj->mBytecodeLink->mBytecode);

				inst(Instruction(OpCode::LOAD_CONST, method_const_obj));
				inst(Instruction(OpCode::LOAD_CONST, mConstants.get(func.mFunctionId)));
				inst(Instruction(OpCode::DEF_LOCAL));
				break;
			}
		case Statement::Type::DEF_LOCAL:
			{
				auto stm_local_def = (StatementLocalDef*) stm;

				if (!stm_local_def->mIsConstExpr) {
					auto const_id = defineLocal(stm_local_def->mLocalId);
					EvalExpr(stm_local_def->mNewExpr);
					inst(Instruction(OpCode::LOAD_CONST, const_id));
					inst(Instruction(OpCode::DEF_LOCAL));

				} else {

					std::string type;
					switch (stm_local_def->mConstExpr->mConstType) {
						case ExpressionConst::BOOL:
							{
								type = "bool";
								break;
							}
						case ExpressionConst::INT:
							{
								type = "int";
								break;
							}
						case ExpressionConst::FLT:
							{
								type = "float";
								break;
							}
						case ExpressionConst::STR:
							{
								type = "str";
								break;
							}
					}

					auto new_expr = new ExpressionNew(type);
					auto defLocalExpr = new StatementLocalDef(stm_local_def->mLocalId, new_expr);

					EvalStatement(defLocalExpr);
					EvalExpr(stm_local_def->mConstExpr);
					inst(Instruction(OpCode::LOAD_LOCAL, mConstants.get(stm_local_def->mLocalId)));
					inst(Instruction(OpCode::OBJ_COPY));

					delete defLocalExpr;
				}
				break;
			}
		case Statement::Type::COPY:
			{
				auto stm_cp = (StatementCopy*) stm;
				EvalExpr(stm_cp->mRight);
				EvalExpr(stm_cp->mLeft);
				inst(Instruction(OpCode::OBJ_COPY));
				break;
			}
		case Statement::Type::PRINT:
			{
				auto stm_prnt = (StatementPrint*) stm;
				EvalExpr(stm_prnt->mTarget);
				inst(Instruction(OpCode::PRINT));
				break;
			}
		case Statement::Type::RET:
			{
				auto stm_ret = (StatementReturn*) stm;
				if (stm_ret->mRet) {
					EvalExpr(stm_ret->mRet);
					inst(Instruction(OpCode::RETURN_OBJ));
				} else {
					inst(Instruction(OpCode::RETURN));
				}
				break;
			}

		default: ASSERT(0)
	}
}

void FunctionDefinition::EvalExpr(Expression* expr) {
	switch (expr->mType) {
		case Expression::Type::BOOLEAN:
			{
				auto boolean = (ExpressionBoolean*) expr;
				if (boolean->mBoolType == ExpressionBoolean::BoolType::NOT) {
					EvalExpr(boolean->mLeft);
					inst(OpCode::NOT);
				} else {
					EvalExpr(boolean->mRight);
					EvalExpr(boolean->mLeft);
					inst(OpCode(boolean->mBoolType));
				}
				break;
			}
		case Expression::Type::FUNC:
			{
				auto func = (ExpressionFunc*) expr;

				// RelAssert(mConstants.mMethods.presents(func->mFuncId) && "No such function");
				inst(Instruction(OpCode::LOAD_LOCAL, mConstants.get(func->mFuncId)));
				break;
			}
		case Expression::Type::ARITHMETICS:
			{
				auto arithmeticExpression = (ExpressionArithmetics*) expr;
				EvalExpr(arithmeticExpression->mRight);
				EvalExpr(arithmeticExpression->mLeft);
				inst(Instruction(arithmeticExpression->mOpType));
				break;
			}
		case Expression::Type::NEW:
			{
				auto create_new = (ExpressionNew*) expr;
				inst(Instruction(OpCode::LOAD_CONST, mConstants.get(create_new->mNewType)));
				inst(Instruction(OpCode::OBJ_CREATE));
				break;
			}
		case Expression::Type::LOCAL:
			{
				auto local = (ExpressionLocal*) expr;
				// RelAssert(mLocals.presents(local->mLocalId) && "undefined local");
				inst(Instruction(OpCode::LOAD_LOCAL, mConstants.get(local->mLocalId)));
				break;
			}
		case Expression::Type::CONST_EXPR:
			{
				auto constObject = (ExpressionConst*) expr;
				switch (constObject->mConstType) {
					case ExpressionConst::STR:
						{
							inst(Instruction(OpCode::LOAD_CONST, mConstants.get(constObject->str)));
							break;
						}
					case ExpressionConst::INT:
						{
							inst(Instruction(OpCode::LOAD_CONST, mConstants.get(constObject->integer)));
							break;
						}
					case ExpressionConst::FLT:
						{
							inst(Instruction(OpCode::LOAD_CONST, mConstants.get(constObject->floating)));
							break;
						}
					case ExpressionConst::BOOL:
						{
							inst(Instruction(OpCode::LOAD_CONST, mConstants.get(constObject->boolean)));
							break;
						}
				}
				break;
			}
		case Expression::Type::CHILD:
			{
				auto child = (ExpressionChild*) expr;
				EvalExpr(child->mParent);
				inst(Instruction(OpCode::LOAD_CONST, mConstants.get(child->mLocalId)));
				inst(Instruction(OpCode::CHILD, child->mMethod, 1));
				break;
			}
		case Expression::Type::SELF:
			{
				inst(Instruction(OpCode::SELF));
				break;
			}
		case Expression::Type::CALL:
			{
				auto call = (ExpressionCall*) expr;
				inst(Instruction(OpCode::PUSH_ARGS, (alni) call->mArgs->mItems.size(), 1));
				for (auto arg : call->mArgs->mItems) {
					EvalExpr(arg.data());
				}
				EvalExpr(call->mParent);
				inst(Instruction(OpCode::CALL));
				break;
			}

		default: ASSERT(0)
	}
}

alni instSize(const Instruction& inst) {
	switch (inst.mInstType) {
		case Instruction::InstType::JUMP:
		case Instruction::InstType::JUMP_IF_NOT:
		case Instruction::InstType::JUMP_IF:
			{
				// 2 bytes for offset 1 byte for instruction
				return 3;
			}
		case Instruction::InstType::PURE_CONST:
			{
				return 2;
			}
		case Instruction::InstType::EXEC:
			{
				alni out = 1;
				switch (inst.mArgType) {
					case Instruction::ArgType::PARAM:
						{
							out += inst.mParamBytes;
							return out;
						}
					case Instruction::ArgType::CONST_ARG:
						{
							out += 2;
							if (inst.mConstData2) {
								out += 2;
							}
							return out;
						}
					case Instruction::ArgType::NO_ARG:
						{
							return out;
						}
					default:
						{
							ASSERT(0)
						}
				}
			}
		case Instruction::InstType::NONE:
			{
				return 0;
			}
		default:
			{
				ASSERT(0)
			}
	}

	ASSERT(0)
	return 0;
}

void writeConst(ByteCode& out, alni& idx, uint2 data) {
	for (auto byte : IterRange(sizeof(uint2))) {
		out.mInstructions[idx] = OpCode((int1) (data >> byte * 8));
		idx++;
	}
}

void writeParam(ByteCode& out, alni& idx, const int1* data, alni size) {
	for (auto byte : IterRange(size)) {
		out.mInstructions[idx] = OpCode(data[byte]);
		idx++;
	}
}

alni calcOffset(List<Instruction>::Node* jump_inst, Instruction* to) {
	alni offset = 0;
	bool reversed = jump_inst->data.mInstIdx > to->mInstIdx;
	auto iter_node = reversed ? jump_inst : jump_inst->next;
	while (&iter_node->data != to) {
		offset += instSize(iter_node->data);
		iter_node = reversed ? iter_node->prev : iter_node->next;
	}
	if (reversed) {
		offset += instSize(iter_node->data);
	}
	return reversed ? -offset : offset;
}

void FunctionDefinition::generateByteCode(ByteCode& out) {

	out.~ByteCode();
	new (&out) ByteCode();

	mConstants.save(out.mConstants);

	out.updateRefCount();

	alni inst_len = 0;
	for (auto inst_iter : mInstructions) {
		inst_len += instSize(inst_iter.data());
	}
	out.mInstructions.reserve(inst_len);

	alni idx = 0;
	for (auto inst_iter : mInstructions) {
		auto inst = inst_iter.data();

		switch (inst.mInstType) {
			case Instruction::InstType::JUMP_IF:
			case Instruction::InstType::JUMP_IF_NOT:
			case Instruction::InstType::JUMP:
				{
					alni offset = calcOffset(inst_iter.node(), inst.mInstTarget);

					if (offset == 0) {
						out.mInstructions[idx] = OpCode::NONE;
						idx += 3;
						break;
					}

					if (inst.mInstType == Instruction::InstType::JUMP_IF) {
						if (offset > 0) {
							out.mInstructions[idx] = OpCode::JUMP_IF;
						} else {
							out.mInstructions[idx] = OpCode::JUMP_IF_R;
						}
					} else if (inst.mInstType == Instruction::InstType::JUMP_IF_NOT) {
						if (offset > 0) {
							out.mInstructions[idx] = OpCode::JUMP_IF_NOT;
						} else {
							out.mInstructions[idx] = OpCode::JUMP_IF_NOT_R;
						}
					} else {
						if (offset > 0) {
							out.mInstructions[idx] = OpCode::JUMP;
						} else {
							out.mInstructions[idx] = OpCode::JUMP_R;
						}
					}

					idx++;

					alni offset_mod = abs(offset);
					auto offset_param = (uint2) offset_mod;
					writeParam(out, idx, (int1*) &offset_param, 2);
					break;
				}
			case Instruction::InstType::PURE_CONST:
				{
					writeConst(out, idx, (uint2) inst.mConstData->mConstIdx);
					break;
				}
			case Instruction::InstType::EXEC:
				{
					out.mInstructions[idx] = inst.mOp;
					idx++;
					switch (inst.mArgType) {
						case Instruction::ArgType::PARAM:
							{
								writeParam(out, idx, (int1*) &inst.mParam, inst.mParamBytes);
								break;
							}
						case Instruction::ArgType::CONST_ARG:
							{
								writeConst(out, idx, (uint2) inst.mConstData->mConstIdx);
								if (inst.mConstData2) {
									writeConst(out, idx, (uint2) inst.mConstData2->mConstIdx);
								}
								break;
							}
						case Instruction::ArgType::NO_ARG:
							{
								break;
							}
						default:
							{
								ASSERT(0)
							}
					}
					break;
				}
			default:
				{
					ASSERT(0)
				}
			case Instruction::InstType::NONE:
				{
				}
		}
	}
}

List<Instruction>::Node* FunctionDefinition::inst(Instruction inst) {
	mInstructions.pushBack(inst);
	auto out = &mInstructions.lastNode()->data;
	out->mInstIdx = (alni) mInstructions.length() - 1;
	return mInstructions.lastNode();
}

void obj::initialize() {}

void obj::finalize() {}

bool obj::Compile(obj::MethodObject* method) {

	Parser parser;

	auto script = method->mBytecodeLink->mReadable->val;
	auto res = parser.parse(script);

	if (res.isError) {
		// TODO : print parse error
		return false;
	}

	Generate(method->mBytecodeLink->mBytecode, res.scope);

	delete res.scope;

	return true;
}
