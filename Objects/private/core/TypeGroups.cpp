
#include "core/TypeGroups.hpp"

#include "core/Object.hpp"

using namespace obj;

obj::TypeGroups::TypeGroups() :
	is_group(true) {
	new (&childs) Dict();
}

obj::TypeGroups::TypeGroups(bool is_group) :
	is_group(is_group) {
	if (is_group) {
		new (&childs) Dict();
	} else {
		type = nullptr;
	}
}

bool obj::TypeGroups::isGroup() { return is_group; }

obj::TypeGroups::Dict* obj::TypeGroups::getChilds() {
	DEBUG_ASSERT(is_group);
	return &childs;
}

void obj::TypeGroups::setType(ObjectType* type) {
	DEBUG_ASSERT(!is_group);
	this->type = type;
}

void obj::TypeGroups::addType(ObjectType* type, tp::InitialierList<const char*> path, tp::alni cur_dir_idx) {
	DEBUG_ASSERT(is_group);

	tp::alni dir_len = (tp::alni) path.size();

	if (dir_len == cur_dir_idx) {
		TypeGroups* type_ref = new TypeGroups(false);
		type_ref->setType(type);
		childs.put(type->name, type_ref);
		return;
	}

	TypeGroups* group = nullptr;
	tp::alni index = 0;

	for (auto dir : path) {
		if (index != cur_dir_idx) {
			index++;
			continue;
		}

		auto child_idx = childs.presents(dir);
		if (child_idx) {
			group = childs.getSlotVal(child_idx);
		} else {
			group = new TypeGroups(true);
			childs.put(dir, group);
		}

		group->addType(type, path, cur_dir_idx + 1);

		break;
	}
}

const obj::ObjectType* obj::TypeGroups::getType() {
	DEBUG_ASSERT(!is_group);
	return type;
}

obj::TypeGroups::~TypeGroups() {
	if (is_group) {
		for (auto& child : childs) {
			delete child->val;
		}
		childs.~Map();
	}
}
