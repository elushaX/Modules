#include "FloatingWidget.hpp"

namespace tp {
	class DockSpaceWidget : public Widget {
	public:
		enum Side { LEFT, TOP, RIGHT, BOTTOM };

	public:
		DockSpaceWidget();

		void eventProcess(const Events& events) override;
		void eventDraw(Canvas& canvas) override;

		void addSideWidget(Widget* widget, Side side);
		void removeSideWidget(Side side);

		void toggleHiddenState(DockSpaceWidget::Side side);

		void setCenterWidget(Widget* widget);

	private:
		void updateActiveWindow(const tp::Events& events);
		void calculateSideAreas();
		void calculateResizeHandles();
		void handleResizeEvents(const Events& events);
		void updateChildSideWidgets();

		void calculateHeaderAreas();


		bool isSideVisible(Side side);
		bool isSideExists(Side side);
		ualni getVisibleSidesSize();

	private:
		// todo : make better grouping into structs

		Widget* mSideWidgets[4] { nullptr, nullptr, nullptr, nullptr };
		Widget* mCenterWidget = nullptr;

		alni mSideSplitOrder[4] = { -1, -1, -1, -1 };
		halnf mSideAbsoluteSize[4] { 200, 100, 200, 100 };
		bool mSideHidden[4] { true, false, false, false };
		halnf mSideSizePadding = 50.f;

		// Geometry cache
		struct ResizeHandle {
			RectF area{ 0, 0, 0, 0 };
			Side side{ TOP };
			halnf start{ 0 };
			halnf end{ 0 };
			bool active = false;
			bool hover = false;
		};

		Buffer<ResizeHandle> mResizeHandles;
		RectF mSideAreas[4];
		RectF mSideHeaderAreas[4];
		RectF mCenterArea {};

		// View Parameters
		halnf mPadding = 4;
		halnf mHeaderSize = 27;

		RGBA mResizeHandleColorHovered = RGBA(0.3, 0.3, 0.3, 1);
		RGBA mResizeHandleColorActive = RGBA(0.6, 0.6, 0.6, 1);
		RGBA mBackgroundColor = RGBA(0, 0, 0, 1);
	};
}