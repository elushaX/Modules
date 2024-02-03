
#pragma once

#include "AutomataGraph.h"

namespace tp {
	extern ModuleManifest gModuleTokenizer;
}

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

		AstNode() = default;
		virtual ~AstNode() = default;
	};

	template <typename tAlphabetType>
	struct AstVal : public AstNode {
		explicit AstVal(tAlphabetType val) :
			mVal(val) {
			mType = VAL;
		}
		~AstVal() override = default;
		tAlphabetType mVal;
	};

	struct AstCompound : public AstNode {
		AstCompound() { mType = COMPOUND; }
		~AstCompound() override {
			for (auto iter : mChilds) {
				delete iter.data();
			}
			mChilds.removeAll();
		}
		List<AstNode*> mChilds;
	};

	struct AstAlternation : public AstNode {
		AstAlternation() { mType = OR; }
		~AstAlternation() override {
			delete mFirst;
			delete mSecond;
		}
		AstNode* mFirst = nullptr;
		AstNode* mSecond = nullptr;
	};

	struct AstIf : public AstNode {
		AstIf() { mType = IF; }
		~AstIf() override { delete mNode; }
		AstNode* mNode = nullptr;
	};

	struct AstAny : public AstNode {
		AstAny() { mType = ANY; }
		~AstAny() override = default;
	};

	struct AstRepetition : public AstNode {
		AstRepetition() { mType = REPEAT; }
		~AstRepetition() override { delete mNode; }
		AstNode* mNode = nullptr;
		bool mPlus = false;
	};

	template <typename tAlphabetType>
	struct AstClass : public AstNode {
		AstClass() { mType = CLASS; }
		~AstClass() override { mRanges.removeAll(); }
		List<Range<tAlphabetType>> mRanges;
		bool mExclude = false;
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
				case TOK_NONE:
					{
						discardTok();
						return nullptr;
					};
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
				default:
					{
						discardTok();
					}
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
			} else {
				goto READ_VAL;
			}
		}

		AstAny* parseAny() { return new AstAny(); }

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

		void genError(const char* desc) { mError = { desc, mOffset }; }

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
			} else {
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

	template <typename tAlphabetType, typename tStateType, tStateType tNoStateVal, tStateType tFailedStateVal>
	CompileError<tStateType> compile(
		NFA<tAlphabetType, tStateType, tNoStateVal, tFailedStateVal>& out, const tAlphabetType* regex, tStateType state
	) {
		Compiler<tAlphabetType, tStateType, tNoStateVal, tFailedStateVal> compiler;
		compiler.compile(out, regex, state);
		return compiler.mError;
	}

	template <typename tAlphabetType, typename tStateType, tStateType tNoStateVal, tStateType tFailedStateVal>
	CompileError<tStateType> compile(
		NFA<tAlphabetType, tStateType, tNoStateVal, tFailedStateVal>& out,
		const InitialierList<Pair<const tAlphabetType*, tStateType>>& rules
	) {
		Compiler<tAlphabetType, tStateType, tNoStateVal, tFailedStateVal> compiler;
		compiler.compile(out, rules);
		return compiler.mError;
	}
}