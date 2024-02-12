
#pragma once

#include "Environment.hpp"
#include "TypeInfo.hpp"
#include <initializer_list>
#include <typeinfo>
#include <utility>

namespace tp {
	template <typename Type>
	using InitialierList = std::initializer_list<Type>;

	// Selects whether to pass by constant reference or by value
	template <typename tType>
	using SelectValueOrReference = typename TypeSelect<(sizeof(tType) > sizeof(tp::alni)), const tType&, tType>::Result;

	template <typename tType>
	using VoidType = void;

	template <typename tType>
	struct Reference {
		tType& operator()() {}
	};

	template <typename tType>
	struct DeclareValue {
		tType operator()() {}
	};

	struct TrueType {
		static constexpr auto value = true;
	};

	struct FalseType {
		static constexpr auto value = false;
	};
}

namespace tp {
	ualni next2pow(ualni v);
	uhalni next2pow(uhalni v);
	ufalni next2pow(ufalni v);

	ualni hash(const char* bytes);
	ualni hash(alni bytes);
	ualni hash(halni bytes);
	ualni hash(uhalni bytes);
	ualni hash(ualni bytes);
	ualni hash(alnf bytes);

	template <typename T>
	[[nodiscard]] T clamp(T v, T l, T u) {
		if (v < l) {
			v = l;
		} else if (v > u) {
			v = u;
		}
		return v;
	}

	template <typename T>
	[[nodiscard]] T max(T a, T b) {
		return (a > b) ? a : b;
	}

	template <typename T>
	[[nodiscard]] T min(T a, T b) {
		return (a < b) ? a : b;
	}

	template <typename T>
	[[nodiscard]] T abs(T v) {
		if (v < 0) {
			return -v;
		}
		return v;
	}

	template <typename T>
	inline void swap(T& t1, T& t2) {
		const T tmp = t1;
		t1 = t2;
		t2 = tmp;
	}

	// only for x > 0 and y > 0
	template <typename T>
	[[nodiscard]] T ceil_positive(T x, T y) {
		return T(1 + ((x - 1) / (tp::alnf) y));
	}

	// power
	template <typename T>
	[[nodiscard]] T pow(T x, uhalni n) {
		if (n == 0) return 1;
		T out = x;
		for (uhalni i = 0; i < n - 1; i++) {
			out = out * x;
		}
		return out;
	}

	template <typename Type>
	halni nDig10(Type val) {
		val = abs(val);
		halni out = 0;
		while (val != 0) {
			val = halni(val / 10);
			out++;
		}
		return out;
	}
}