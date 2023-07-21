
#pragma once

#include "RegularExpression.h"
#include "Strings.hpp"

namespace tp {

	extern ModuleManifest gModuleTokenizer;

	template <typename tAlphabetType, typename tTokType, tTokType tNoTokVal, tTokType tFailedTokVal>
	class Tokenizer {

		TransitionMatrix<tAlphabetType, tTokType, tNoTokVal, tFailedTokVal> mTransitionMatrix;

		RegEx::CompileError<tTokType> mError;

		bool scanFailed() {
			return mTransitionMatrix.isTrapped();
		}

	public:

		Tokenizer() {
			MODULE_SANITY_CHECK(gModuleTokenizer)
		}

		void build(const init_list<Pair<const tAlphabetType*, tTokType>>& rules) {
			NFA<tAlphabetType, tTokType, tNoTokVal, tFailedTokVal> nfa;
			
			mError = RegEx::compile(nfa, rules);
			if (mError.isError()) {
				return;
			}

			DFA<tAlphabetType, tTokType, tNoTokVal, tFailedTokVal> dfa(nfa);
			mTransitionMatrix.construct(dfa);
		}

		[[nodiscard]] bool isBuild() const {
			return !mError.isError();
		}

		const RegEx::CompileError<tTokType>& getBuildError() {
			return mError;
		}

		void resetMatrix() {
			mTransitionMatrix.reset();
		}

		tTokType advanceSymbol(tAlphabetType symbol) {
			return mTransitionMatrix.move(symbol);
		}

		tTokType advanceToken(const tAlphabetType* source, ualni source_len, ualni* token_len) {
			tTokType out = tNoTokVal;
			*token_len = 0;
			for (ualni idx = 0; idx < source_len; idx++) {
				out = advanceSymbol(source[idx]);
				if (out != tNoTokVal) {
					*token_len = idx;
					break;
				}
			}
			return out;
		}

		~Tokenizer() = default;
	};

	template <typename tAlphabetType, typename tTokType, tTokType tNoTokVal, tTokType tFailedTokVal, tTokType tSourceEndTokVal>
	class SimpleTokenizer {
		
		Tokenizer<tAlphabetType, tTokType, tNoTokVal, tFailedTokVal> mTokenizer;

		const tAlphabetType* mSource = nullptr;
		ualni mLastTokLen = 0;
		ualni mSourceLen = 0;
		ualni mAdvancedOffset = 0;

	public:

		struct Cursor {
			const tAlphabetType* mSource = nullptr;
			ualni mAdvancedOffset = 0;
			const tAlphabetType* str() { return mSource + mAdvancedOffset; }
		};

		SimpleTokenizer() = default;

		void build(const init_list<Pair<const tAlphabetType*, tTokType>>& rules) {
			mTokenizer.build(rules);
		}

		[[nodiscard]] bool isBuild() const {
			return mTokenizer.isBuild();
		}

		const RegEx::CompileError<tTokType>& getBuildError() {
			return mTokenizer.getBuildError();
		}

		void bindSource(const tAlphabetType* source) {
			mSource = source;
			while (mSource[mSourceLen]) mSourceLen++;
			mSourceLen++;
		}

		[[nodiscard]] bool isInputLeft() const {
			if (!mSource) {
				return false;
			}
			if (mSource[mAdvancedOffset] == nullptr) {
				return false;
			}
			return true;
		}

		Cursor getCursor() const {
			return { mSource, mAdvancedOffset };
		}

		Cursor getCursorPrev() const {
			return { mSource, mAdvancedOffset - mLastTokLen };
		}

		void setCursor(const Cursor& crs) {
			mAdvancedOffset = crs.mAdvancedOffset;
			mLastTokLen = 0;
		}

		tTokType readTok() {
			if (mSourceLen == mAdvancedOffset + 1) {
				return tSourceEndTokVal;
			}

			tTokType out = mTokenizer.advanceToken(mSource + mAdvancedOffset, mSourceLen - mAdvancedOffset, &mLastTokLen);
			mAdvancedOffset += mLastTokLen;
			return out;
		}

		tTokType lookupTok() {
			auto out = readTok();
			discardTok();
			return out;
		}

		void discardTok() {
			mTokenizer.resetMatrix();
			mAdvancedOffset -= mLastTokLen;
			mLastTokLen = 0;
		}

		void skipTok() {
			readTok();
		}

		void reset() {
			mAdvancedOffset = mLastTokLen = 0;
			mTokenizer.resetMatrix();
		}

		[[nodiscard]] ualni lastTokLEn() const {
			return mLastTokLen;
		}

		String extractVal() {
			auto crs = getCursorPrev();
			String out;
			out.resize(mLastTokLen);
			memCopy(out.write(), crs.str(), mLastTokLen);
			return out;
		}
	};
}