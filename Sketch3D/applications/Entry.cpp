
#include "nodes.h"
#include "StrokesWidget.h"

int main(int argc, char* argv[]) {
	
	auto nodes = (argc == 2) ? nd::NodesCore::createFast(argv[1]) : nd::NodesCore::createFast();

	tp::init_utils();

	obj::NDO->define(&obj::StrokesObject::TypeData);
	obj::NDO->define(&nd::StrokesWidget::TypeData);
	obj::NDO->type_groups.addType(&obj::StrokesObject::TypeData, { "Strokes", "StrokesProject" });
	obj::NDO->type_groups.addType(&nd::StrokesWidget::TypeData, { "Strokes", "StrokesWidget" });

	nd::StrokesWidget::OpsInit();
	nd::StrokesWidget::addShortcuts(nodes);

	auto root_widget = NDO_CAST(nd::Widget, obj::NDO->create("RootWidget"));

	auto childs = root_widget->getMember<obj::ListObject>("childs");
	
	childs->pushBack(obj::NDO->create("StrokesWidget"));
	
	nodes->bindRootWindow(root_widget);

	nodes->run();

	nd::StrokesWidget::OpsUnInit();
	tp::finalize_utils();

	nodes->destroyFast();
	tp::terminate();
}