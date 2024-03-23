
#pragma once

#include "Map.hpp"
#include <string>

namespace tp::obj {

	struct ObjectType;
	struct ObjectsContext;

	class TypeGroups {

	public:

		typedef Map<std::string, TypeGroups*> Dict;

		TypeGroups();

		friend struct ObjectsContext;

		explicit TypeGroups(bool is_group);

		void addType(ObjectType* type, InitialierList<const char*> path, alni cur_arg = 0);
		void setType(ObjectType* type);
		bool isGroup();
		Dict* getChilds();
		const obj::ObjectType* getType();
		~TypeGroups();

	private:

		bool is_group;
		union {
			Dict childs;
			const obj::ObjectType* type;
		};
	};
}