#pragma once

#include "core/Object.hpp"
#include "Map.hpp"
#include "List.hpp"

namespace tp::obj {

	struct Scope {
		typedef Map<std::string, Object*> ObjDict;
		typedef List<Object*> ObjList;

		ObjDict mLocals;
		ObjList mTemps;
		bool mChildReachable = true;

		~Scope();
	};

	class ScopeStack {

		enum : alni { MAX_STACK_SIZE = 1024 * 4 };

	public:
		Scope* mBuff;
		ualni mIdx;
		ualni mIterIdx;

		ScopeStack();
		void enterScope(bool aChildReachable);
		void leaveScope();
		void addLocal(Object* local, const std::string& id);
		void addTemp(Object* tmp);
		void popTemp();
		void addTempReturn(Object* ret);
		Object* getLocal(const std::string& str);
		Scope* getCurrentScope();
		~ScopeStack();

	private:
		Object* getLocalUtil(Scope& scope, const std::string& id);
	};
}