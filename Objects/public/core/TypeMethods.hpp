
#pragma once

#include "Buffer.hpp"
#include <string>

namespace tp::obj {

	struct Interpreter;
	struct Object;
	struct ObjectType;

	struct TypeMethod {
		enum { MAX_ARGS = 8 };
		struct Arg {
			const char* descr = nullptr;
			mutable Object* obj = nullptr;
		};

		const char* nameid = nullptr;
		const char* descr = nullptr;

		mutable Object* self = nullptr;
		Arg args[MAX_ARGS];
		
		int1 mNargs = 0;

		void (*exec)(const TypeMethod* tm) = nullptr;

		mutable Arg ret;

		void operator()(Interpreter* interp) const;

		void init();
	};

	extern TypeMethod gDefaultTypeMethods[3];

	struct TypeMethods {
		enum : int2 { MAX_TYPE_METHODS = 128 };

		TypeMethod* methods[MAX_TYPE_METHODS];
		halni mNMethods = 0;

		struct LookupKey {
			int2 key = -1;
			int2 type_depth = -1;
			operator bool() { return key != -1; }
		};

		static LookupKey presents(const ObjectType*, const std::string&);
		static const TypeMethod* getMethod(const ObjectType*, LookupKey);

		void init();

		halni nMethods() const;

	private:
		int2 presents(const std::string&) const;
		const TypeMethod* getMethod(int2) const;
	};
}