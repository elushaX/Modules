#include "LayoutManager.hpp"

#include "Widget.hpp"

using namespace tp;

void LayoutManager::adjust(Widget* root) {
	Widget::dfs(
		root,
		[](Widget* widget) {
			widget->getLayout()->pickRect();
		},
		[](Widget* widget) {
			widget->getLayout()->adjustChildrenRect();
		}
	);
}