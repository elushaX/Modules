
#include "gui/Widgets.h"

#include "nodes.h"
#include "StrokesObject.h"

namespace nd {
	struct StrokesWidget : Widget {
		static obj::ObjectType TypeData;
		static struct Vtable : Widget::Vtable {} vtable;

		strokes::Renderer mRenderer;
		nd::GUIdrawer::ImageHandle mImage;
		nd::GUIdrawer* mDrawer = NULL;
		
		static void constructor(StrokesWidget* self);
		static void destructor(StrokesWidget* self);
		static void copy(Widget* self, const Widget* in);
		static void procInputs(StrokesWidget* self, nd::GUIInputs* inputs);
		static void presentOutput(StrokesWidget* self, nd::GUIdrawer* drawer);

		strokes::Project* getTargetProject();

		static void OpsInit();
		static void OpsUnInit();

		static void addShortcuts(nd::NodesCore* core);
	};
};