
#pragma once

#include "AutomataGraph.h"

namespace tp::RegEx {
		
		struct AstNode {
			enum Type {
				NONE,
				ANY,
				OR,
				IF,
				CLASS,
				COMPOUND,
				REPEAT,
				VAL,
			} mType = NONE;
		};

		template <typename tAlphabetType>
		struct AstVal : public AstNode {
			explicit AstVal(tAlphabetType val) : mVal(val) { mType = VAL; }
			tAlphabetType mVal;
		};

		struct AstCompound : public AstNode {
			AstCompound() { mType = COMPOUND; }
			List<AstNode*> mChilds;
		};

		struct AstAlternation : public AstNode {
			AstAlternation() { mType = OR;	}
			AstNode* mFirst = nullptr;
			AstNode* mSecond = nullptr;
		};

		struct AstIf : public AstNode {
			AstIf() { mType = IF; }
			AstNode* mNode = nullptr;
		};

		struct AstAny : public AstNode {
			AstAny() { mType = ANY; }
		};

		struct AstRepetition : public AstNode {
			AstRepetition() { mType = REPEAT; }
			AstNode* mNode = nullptr;
			bool mPlus = false;
		};

		template <typename tAlphabetType>
		struct AstClass : public AstNode {
			AstClass() { mType = CLASS; }
			List<Range<tAlphabetType>> mRanges;
			bool mExclude{};
		};

		struct ParseError {
			const char* description = nullptr;
			uhalni offset = 0;
			[[nodiscard]] bool isError() const { return description != nullptr; }
		};

		template <typename tAlphabetType, typename tStateType, tStateType tNoStateVal>
		class Parser {

			enum TokType : uint1 {
				TOK_COMPOUND_START = 0,
				TOK_COMPOUND_END,
				TOK_CLASS_START,
				TOK_CLASS_END,
				TOK_CLASS_START_EXCLUDE,
				TOK_CLASS_END_EXCLUDE,
				TOK_OR,
				TOK_IF,
				TOK_ANY,
				TOK_REPEAT,
				TOK_REPEAT_PLUS,
				TOK_HYPHEN,
				TOK_SPECIALS_END_,
				TOK_VAL,
				TOK_NONE,
			};

			tAlphabetType SpecialSymbols[TOK_SPECIALS_END_] = {
				'(', ')', '[', ']', '{', '}', '|', '?', '.', '*', '+', '-',
			};

			tAlphabetType mEscapeSymbol = '\\';

			struct Token {
				TokType type;
				tAlphabetType val;
			};

			const tAlphabetType* mSource = nullptr;
			uhalni mOffset = 0;
			Token mCurToken;
			uhalni mTokLength = 0;

		public:

			ParseError mError;

			// regular expression must be a zero termination string
			AstCompound* parse(const tAlphabetType* regex) {
				mSource = regex;
				return parseRegEx();
			}

		private:

			AstCompound* parseRegEx() {
				auto out = new AstCompound();
				for (AstNode* node = parseElement(); node; node = parseElement()) {
					out->mChilds.pushBack(node);
				}
				if (!out->mChilds.length()) {
					genError("Expected A Expression");
				}
				if (mError.description) {
					delete out;
					return nullptr;
				}
				return out;
			}

			AstNode* parseElement() {
				AstNode* out = nullptr;
				switch (readTok().type) {
					case TOK_COMPOUND_START: out = parseCompound(); break;
					case TOK_CLASS_START: out = parseClass(); break;
					case TOK_CLASS_START_EXCLUDE: out = parseClass(true); break;
					case TOK_ANY: out = parseAny(); break;
					case TOK_VAL: out = parseVal(); break;
					case TOK_NONE: { discardTok(); return nullptr; };
					default: break;
				}
				if (!out) {
					discardTok();
					return nullptr;
				}
				switch (readTok().type) {
					case TOK_OR: out = parseAlternation(out); break;
					case TOK_REPEAT: out = parseRepetition(out); break;
					case TOK_REPEAT_PLUS: out = parseRepetition(out, true); break;
					case TOK_IF: out = parseIf(out); break;
					case TOK_NONE: break;
					default: { discardTok(); }
				}
				return out;
			}

			AstCompound* parseCompound() {
				auto out = new AstCompound();
				for (AstNode* node = parseElement(); node; node = parseElement()) {
					out->mChilds.pushBack(node);
				}
				if (readTok().type != TOK_COMPOUND_END) {
					genError("Expected Compound End");
				}
				if (mError.description) {
					delete out;
					return nullptr;
				}
				return out;
			}

			AstClass<tAlphabetType>* parseClass(bool exclude = false) {
				auto out = new AstClass<tAlphabetType>();
				out->mExclude = exclude;
				auto& ranges = out->mRanges;

				readTok();

			READ_VAL:
				if (mCurToken.type != TOK_VAL) {
					delete out;
					genError("Expected A Value");
					return nullptr;
				}
				char range_start = mCurToken.val;

				readTok();
				if (mCurToken.type != TOK_HYPHEN) {
					delete out;
					genError("Expected A Range");
					return nullptr;
				}

				readTok();
				if (mCurToken.type != TOK_VAL) {
					delete out;
					genError("Expected A Value");
					return nullptr;
				}
				char range_end = mCurToken.val;

				ranges.pushBack({ range_start, range_end });

				readTok();
				if ((mCurToken.type == TOK_CLASS_END && !exclude) || (mCurToken.type == TOK_CLASS_END_EXCLUDE && exclude)) {
					return out;
				}
				else {
					goto READ_VAL;
				}
			}

			AstAny* parseAny() {
				return new AstAny();
			}

			AstVal<tAlphabetType>* parseVal() {
				auto out = new AstVal<tAlphabetType>(mCurToken.val);
				return out;
			}

			AstAlternation* parseAlternation(AstNode* left) {
				auto right = parseElement();
				if (!right) {
					genError("Expected Alternation right Side");
					delete left;
					return nullptr;
				}

				auto out = new AstAlternation();

				out->mFirst = left;
				out->mSecond = right;

				return out;
			}

			AstRepetition* parseRepetition(AstNode* left, bool plus = false) {
				auto out = new AstRepetition();
				out->mNode = left;
				out->mPlus = plus;
				return out;
			}

			AstIf* parseIf(AstNode* left) {
				auto out = new AstIf();
				out->mNode = left;
				return out;
			}

			void genError(const char* desc) {
				mError = { desc, mOffset };
			}

			Token& readTok() {

				const tAlphabetType* crs = mSource + mOffset;

				// zero termination string
				if (*crs == 0) {
					mCurToken.type = TOK_NONE;
					return mCurToken;
				}

				mTokLength = 1;
				mCurToken.type = TOK_VAL;
				mCurToken.val = crs[0];

				if (crs[0] == mEscapeSymbol) {
					mCurToken.val = crs[1];
					mTokLength = 2;
				}
				else {
					for (uhalni tok = 0; tok < TOK_SPECIALS_END_; tok++) {
						if (SpecialSymbols[tok] == mCurToken.val) {
							mCurToken.type = TokType(tok);
							break;
						}
					}
				}

				mOffset += mTokLength;
				return mCurToken;
			}

			void discardTok() {
				mOffset -= mTokLength;
				mTokLength = 0;
			}
		};
		
		template <typename tStateType>
		struct CompileError {
			ParseError mParseError;
			uhalni mRuleIndex = 0;
			tStateType mRuleState;
			const char* description = nullptr;
			[[nodiscard]] bool isError() const { return description; }
		};

		template <typename tAlphabetType, typename tStateType, tStateType tNoStateVal, tStateType tFailedStateVal>
		class Compiler {

			typedef NFA<tAlphabetType, tStateType, tNoStateVal, tFailedStateVal> Graph;
			typedef typename Graph::Vertex Vertex;
			typedef Parser<tAlphabetType, tStateType, tNoStateVal> Parser;

			struct Node {
				Vertex* left = nullptr;
				Vertex* right = nullptr;
			};

		private:
			Graph* mGraph = nullptr;

		public:
			CompileError<tStateType> mError;

			Node compile(Graph& graph, const tAlphabetType* regex, tStateType state) {
				mGraph = &graph;
				return compileUtil(regex, state);
			}

			Node compile(Graph& aGraph, init_list<Pair<const tAlphabetType*, tStateType>> aRules) {
				mGraph = &aGraph;

				auto left = mGraph->addVertex();
				auto right = mGraph->addVertex();

				halni idx = 0;
				for (auto rule: aRules) {

					auto node = idx ? compileUtil(rule.head, rule.tail) : compileUtil(rule.head, rule.tail, left, right);

					if (!(node.left && node.right)) {
						mError.mRuleIndex = idx;
						return {};
					}

					if (idx) {
						transitionAny(left, node.left);
						transitionAny(node.right, right);
					}

					idx++;
				}

				mGraph->setStartVertex(left);

				return { left, right };
			}

		private:

			Node compileUtil(const tAlphabetType* regex, tStateType state, Vertex* aLeft = nullptr, Vertex* aRight = nullptr) {
				Parser parser;
				auto astNode = parser.parse(regex);
				if (parser.mError.isError()) {
					mGraph->setStartVertex(nullptr);
					mError.description = "Parsing Of Regular Expression Failed";
					mError.mRuleState = state;
					mError.mParseError = parser.mError;
					return {};
				}

				auto node = compileNode(astNode, aLeft, aRight);
				delete astNode;

				mGraph->setVertexState(node.right, state);
				mGraph->setStartVertex(node.left);

				return node;
			}

			Node compileVal(AstVal<tAlphabetType>* val, Vertex* aLeft = nullptr, Vertex* aRight = nullptr) {
				auto left = aLeft ? aLeft : mGraph->addVertex();
				auto right = aRight ? aRight : mGraph->addVertex();
				transitionVal(left, right, val->mVal);
				return { left, right };
			}

			Node compileAlternation(AstAlternation* alt, Vertex* aLeft = nullptr, Vertex* aRight = nullptr) {
				auto first_node = compileNode(alt->mFirst, aLeft, aRight);
				auto second_node = compileNode(alt->mSecond);
				transitionAny(first_node.left, second_node.left);
				transitionAny(second_node.right, first_node.right);
				return first_node;
			}

			Node compileAny(AstAny*, Vertex* aLeft = nullptr, Vertex* aRight = nullptr) {
				auto left = aLeft ? aLeft : mGraph->addVertex();
				auto right = aRight ? aRight : mGraph->addVertex();
				transitionAny(left, right, true);
				return { left, right };
			}

			Node compileRepeat(AstRepetition* repeat, Vertex* aLeft = nullptr, Vertex* aRight = nullptr) {
				if (repeat->mPlus) {
					auto middle = mGraph->addVertex();

					auto left_node = compileNode(repeat->mNode, aLeft, middle);

					auto right_node = compileNode(repeat->mNode, middle, aRight);
					transitionAny(right_node.right, right_node.left);
					transitionAny(right_node.left, right_node.right);

					return { left_node.left, right_node.right };
				}
				else {
					auto node = compileNode(repeat->mNode, aLeft, aRight);
					transitionAny(node.right, node.left);
					transitionAny(node.left, node.right);
					return node;
				}
			}

			Node compileIf(AstIf* ifNode, Vertex* aLeft = nullptr, Vertex* aRight = nullptr) {
				auto node = compileNode(ifNode->mNode, aLeft, aRight);
				transitionAny(node.left, node.right);
				return node;
			}

			Node compileClass(AstClass<tAlphabetType>* node, Vertex* aLeft = nullptr, Vertex* aRight = nullptr) {
				auto left = aLeft ? aLeft : mGraph->addVertex();
				auto right = aRight ? aRight : mGraph->addVertex();
				
				if (node->mRanges.length() == 1) {
					auto const& range = node->mRanges.first()->data;
					transitionRange(left, right, { range.mBegin, range.mEnd }, node->mExclude);
					return { left, right };
				}

				for (auto range : node->mRanges) {
					auto middle = mGraph->addVertex();
					transitionRange(left, middle, { range.data().mBegin, range.data().mEnd }, node->mExclude);
					transitionAny(middle, right);
				}
				return { left, right };
			}

			Node compileCompound(AstCompound* compound, Vertex* aLeft = nullptr, Vertex* aRight = nullptr) {
				Vertex* left = nullptr;
				Vertex* rigth = nullptr;

				ualni idx = 0;
				for (auto child : compound->mChilds) {
					auto pass_left = idx == 0 ? aLeft : rigth;
					auto pass_right = idx == compound->mChilds.length() - 1 ? aRight : nullptr;
					auto node = compileNode(child.data(), pass_left, pass_right);
					if (!left) left = node.left;
					rigth = node.right;
					idx++;
				}

				return { left, rigth };
			}

			Node compileNode(AstNode* node, Vertex* aLeft = nullptr, Vertex* aRight = nullptr) {
				switch (node->mType) {
					case AstNode::CLASS: return compileClass((AstClass<tAlphabetType>*)node, aLeft, aRight);
					case AstNode::COMPOUND: return compileCompound((AstCompound*)node, aLeft, aRight);
					case AstNode::IF: return compileIf((AstIf*)node, aLeft, aRight);
					case AstNode::REPEAT: return compileRepeat((AstRepetition*)node, aLeft, aRight);
					case AstNode::ANY: return compileAny((AstAny*)node, aLeft, aRight);
					case AstNode::OR: return compileAlternation((AstAlternation*)node, aLeft, aRight);
					case AstNode::VAL: return compileVal((AstVal<tAlphabetType>*)node, aLeft, aRight);
					case AstNode::NONE:
						break;
				}
				ASSERT(0)
				return {};
			}

			void transitionAny(Vertex* from, Vertex* to, bool consumes = false) {
				mGraph->addTransition(from, to, {}, consumes, true, false);
			}

			void transitionVal(Vertex* from, Vertex* to, tAlphabetType val) {
				mGraph->addTransition(from, to, { val, val }, true, false, false);
			}

			void transitionRange(Vertex* from, Vertex* to, Range<tAlphabetType> range, bool exclude) {
				mGraph->addTransition(from, to, range, true, false, exclude);
			}
		};

		template <typename tAlphabetType, typename tStateType, tStateType tNoStateVal, tStateType tFailedStateVal>
		CompileError<tStateType>	compile(NFA<tAlphabetType, tStateType, tNoStateVal, tFailedStateVal>& out, const tAlphabetType* regex, tStateType state) {
			Compiler<tAlphabetType, tStateType, tNoStateVal, tFailedStateVal> compiler;
			compiler.compile(out, regex, state);
			return compiler.mError;
		}

		template <typename tAlphabetType, typename tStateType, tStateType tNoStateVal, tStateType tFailedStateVal>
		CompileError<tStateType>	compile(NFA<tAlphabetType, tStateType, tNoStateVal, tFailedStateVal>& out, const init_list<Pair<const tAlphabetType*, tStateType>>& rules) {
			Compiler<tAlphabetType, tStateType, tNoStateVal, tFailedStateVal> compiler;
			compiler.compile(out, rules);
			return compiler.mError;
		}
	}