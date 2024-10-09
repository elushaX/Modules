#include "Widget.hpp"

namespace tp {
	class DockLayoutWidget : public Widget {
	public:
		enum Side { LEFT, TOP, RIGHT, BOTTOM, NONE  };

	private:
		struct ResizeHandle {
			RectF area{ 0, 0, 0, 0 };
			halnf start{ 0 };
			halnf end{ 0 };
			bool active = false;
			bool hover = false;
		};

		struct SideWidgetData {
			Widget* widget = nullptr;
			bool hidden = false;
			halnf absoluteSize = 200;
			alni order = -1;

			Side side = { TOP };

			RectF area = {};
			RectF headerArea = {};
			RectF previewHandleArea = {};

			ResizeHandle resizeHandle;
		};

	public:
		DockLayoutWidget();

	public:
		void adjustRect() override {}
		void adjustChildrenRect() override {}

		void process(const EventHandler& events) override;
		void draw(Canvas& canvas) override;
		void drawOverlay(Canvas& canvas) override;

		[[nodiscard]] bool propagateEventsToChildren() const override;
		[[nodiscard]] bool needsNextFrame() const override;

	public:
		void setCenterWidget(Widget* widget);

		void dockWidget(Widget* widget, Side side);
		void undockWidget(Side side);
		void toggleWidgetVisibility(Side side);

	private:
		void calculateSideAreas();
		void calculateResizeHandles();
		void handleResizeEvents(const EventHandler& events);
		void updateChildSideWidgets();

		void calculateHeaderAreas();

		bool isSideVisible(Side side);
		bool sideExists(DockLayoutWidget::Side side);
		ualni getVisibleSidesSize();
		void handlePreview(const EventHandler& events);

	private:
		RectF mPreviewArea = {};
		Side mPreviewSide = NONE;
		int resizeType[2] = { 0, 0 };

		SideWidgetData mSideWidgets[4];

	private:
		bool mResizing = false;

		Widget* mCenterWidget = nullptr;
		RectF mCenterArea {};

		// Parameters
		halnf mSideSizePadding = 150.f;
		halnf mPadding = 4;
		halnf mHeaderSize = 27;
		halnf mRounding = 10;
		Vec2F mPreviewHandleSize = { 50, 50 };

		RGBA mResizeHandleColorHovered = RGBA(0.3, 0.3, 0.3, 1);
		RGBA mResizeHandleColorActive = RGBA(0.6, 0.6, 0.6, 1);
		RGBA mBackgroundColor = RGBA(0, 0, 0, 1);
		RGBA mPreviewColor = RGBA(0.6, 0.6, 0.6, 1);

	public:
		bool mPreview = false;
	};
}