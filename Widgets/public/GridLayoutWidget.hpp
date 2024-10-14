#include "WidgetBase.hpp"

namespace tp {
	class DockWidget : public Widget {
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
		DockWidget();

		void eventProcess(const Events& events) override;
		void eventDraw(Canvas& canvas) override;
		void eventDrawOver(Canvas& canvas) override;

		void addSideWidget(Widget* widget, Side side);
		void removeSideWidget(Side side);

		void toggleHiddenState(Side side);

		void setCenterWidget(Widget* widget);

		Side getPreviewSide();

	private:
		void calculateSideAreas();
		void calculateResizeHandles();
		void handleResizeEvents(const Events& events);
		void updateChildSideWidgets();

		void calculateHeaderAreas();

		bool isSideVisible(Side side);
		bool sideExists(DockWidget::Side side);
		ualni getVisibleSidesSize();
		void handlePreview(const Events& events);

	private:
		RectF mPreviewArea = {};
		Side mPreviewSide = NONE;
		int resizeType[2] = { 0, 0 };

	public:
		SideWidgetData mSideWidgets[4];

	private:
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