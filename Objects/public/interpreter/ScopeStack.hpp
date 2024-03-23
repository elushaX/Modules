#pragma once

#include "core/Object.hpp"
#include "Map.hpp"

namespace obj {

	struct Scope {
		typedef tp::Map<std::string, obj::Object*> ObjDict;
		typedef tp::List<obj::Object*> ObjList;

		ObjDict mLocals;
		ObjList mTemps;
		bool mChildReachable = true;

		~Scope();
	};

	class ScopeStack {

		enum : tp::alni {
			MAX_STACK_SIZE = 1024 * 4
		};

	public:

		Scope* mBuff;
		tp::ualni mIdx;
		tp::ualni mIterIdx;

		ScopeStack();
		void enterScope(bool aChildReachable);
		void leaveScope();
		void addLocal(obj::Object* local, const std::string& id);
		void addTemp(obj::Object* tmp);
		void popTemp();
		void addTempReturn(obj::Object* ret);
		obj::Object* getLocal(const std::string& str);
		Scope* getCurrentScope();
		~ScopeStack();

	private:
		obj::Object* getLocalUtil(Scope& scope, const std::string& id);
	};

};