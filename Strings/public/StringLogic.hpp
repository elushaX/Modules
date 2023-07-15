#pragma once

#include "Allocators.hpp"
#include "Buffer.hpp"
#include "Utils.hpp"
#include <cstdio>
#include <cstdlib>

namespace tp {

	template<typename tChar = uint1>
	class StringLogic {
	public:

		typedef halni Index;
		typedef Index LineId;
		typedef Index ColumnId;

		static inline bool isNewLineChar(tChar in) { return (in == '\n') || (in == '\r'); }
		static inline bool isEndChar(tChar in) { return in == '\0'; }
		static constexpr tChar getEndChar() { return '\0'; }

		static Index calcLength(const tChar* in) {
			const tChar* iter = in;
			while (*iter != '\0') iter++;
			return iter - in;
		}

		static tChar* insertLogic(const tChar* cur, const tChar* tar, Index at, Index len) {
			auto curLen = calcLength(cur);
			auto allLen = curLen + len;
			auto* out = new tChar[allLen + 1];

			DEBUG_ASSERT(curLen >= 0)
			DEBUG_ASSERT(len >= 0)
			DEBUG_ASSERT(at < curLen + 1 && at >= 0)

			for (Index idx = 0; idx < at; idx++) {
				out[idx] = cur[idx];
			}
			for (Index idx = 0; idx < len; idx++) {
				out[idx + at] = tar[idx];
			}
			for (Index idx = at + len; idx < allLen; idx++) {
				out[idx] = cur[idx - len];
			}
			out[allLen] = '\0';
			return out;
		}

		// including end not including start
		static tChar* removeLogic(const tChar* cur, Index start, Index end) {
			auto curLen = calcLength(cur);
			auto rangeLen = end - start;
			auto newLen = curLen - rangeLen;
			auto* out = new tChar[newLen + 1];
			out[newLen] = '\0';
			for (Index idx = 0; idx < start; idx++) {
				out[idx] = cur[idx];
			}
			for (Index idx = end; idx < curLen; idx++) {
				out[idx - rangeLen] = cur[idx];
			}
			return out;
		}

		static bool isEqualLogic(const tChar* left, const tChar* right) {
			Index idx = 0;
		LOOP:
			if (left[idx] == '\0' || right[idx] == '\0') {
				if (left[idx] == '\0' && right[idx] == '\0') {
					return true;
				}
				return false;
			}
			if (left[idx] != right[idx]) {
				return false;
			}
			idx++;
			goto LOOP;
		}

		// returns 0 : nl1 + 1 : nl2 + 1 : nl3 + 1 : ... : size
		static void calcLineOffsets(const tChar* aBuff, Index aLength, Buffer<Index>& aOut) {
			Index lines = 0;
			for (auto idx : Range(aLength)) {
				if (isNewLineChar(aBuff[idx])) {
					lines++;
				}
			}
			aOut.reserve(lines + 2); // plus start and end offsets
			lines = 0;
			for (auto idx : Range(aLength)) {
				if (isNewLineChar(aBuff[idx])) {
					aOut[lines + 1] = Index(idx + 1);
					lines++;
				}
			}
			aOut[0] = 0;
			aOut[lines + 1] = aLength;
		}
		

		static bool convertStringToValue(const tChar* input, alni& output) {
			char* endPtr;
			output = std::strtoll(input, &endPtr, 10);
			return (endPtr == input + calcLength(input));
		}

		static bool convertStringToValue(const tChar* input, alnf& output) {
			char* endPtr;
			output = std::strtod(input, &endPtr);
			return (endPtr == input + calcLength(input));
		}

		static bool convertStringToValue(const tChar* input, bool& output) {
			output = !(isEqualLogic(input, "False") || isEqualLogic(input, "false") || isEqualLogic(input, "0"));
      return true;
		}

		static bool convertValueToString(alni input, tChar* output, Index bufferSize) {
			int result = std::snprintf(output, bufferSize, "%lld", input);
			return result >= 0;
		}
		
		static bool convertValueToString(alnf input, tChar* output, Index bufferSize) {
			int result = std::snprintf(output, bufferSize, "%f", input);
			return result >= 0;
		}

		static bool convertValueToString(bool input, tChar* output, Index bufferSize) {
			const char* str = (input ? "true" : "false");
			Index length = calcLength(str);
			if (length >= bufferSize)
				return false;
			memCopy(output, str, length + 1);
			return true;
		}
	};
}