#include "Widget.hpp"

namespace tp {
	class DockLayout : public WidgetLayout {
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
			halnf absoluteSize = 300;
			alni order = -1;

			Side side = { TOP };

			RectF area = {};
			RectF headerArea = {};
			RectF previewHandleArea = {};

			ResizeHandle resizeHandle;

			RectF areaBeforeDocking = {};
		};

	public:
		explicit DockLayout(Widget* widget);

		void updateResizeHover(const Vec2F& pointer);
		bool startResize(const Vec2F& pointer);
		void updateResize(const Vec2F& pointerDelta);
		void endResize();

		void updatePreviewSide(const Vec2F& pointer);

	public:
		void pickRect(bool vertical) override {}
		void clampRect() override {};
		void updateLayout(bool vertical) override;
		void adjustChildrenRect();

	public:
		bool setCenterWidget(Widget* widget);

		bool dockWidget(Widget* widget, Side side);
		bool undockWidget(Side side);
		void toggleWidgetVisibility(Side side);

	public:
		[[nodiscard]] bool isSideVisible(Side side) const;
		[[nodiscard]] bool sideExists(Side side) const;

		[[nodiscard]] bool isResizing() const { return mResizing; }
		[[nodiscard]] bool isResizing(Side side) const { return mSideWidgets[side].resizeHandle.active; }
		[[nodiscard]] bool isResizeHandleHover(Side side) const { return mSideWidgets[side].resizeHandle.hover; }

		Side getSideFromWidget(Widget*);
		Side getPreviewSide();

		[[nodiscard]] RectF getRectBeforeDocked(Side side) const { return mSideWidgets[side].areaBeforeDocking; }
		[[nodiscard]] RectF getResizeHandleArea(Side side) const { return mSideWidgets[side].resizeHandle.area; }
		[[nodiscard]] RectF getHeaderArea(Side side) const { return mSideWidgets[side].headerArea; }
		[[nodiscard]] RectF getPreviewHandleArea(Side side) const { return mSideWidgets[side].previewHandleArea; }
		[[nodiscard]] RectF getPreviewArea() const { return mPreviewArea; }

		Widget* getSideWidget(Side side) { return mSideWidgets[side].widget; }

		const std::vector<Widget*>& getDockedWidgets();

	private:
		ualni getVisibleSidesSize() const;

		void calculateSideAreas();
		void calculateResizeHandles();
		void updateChildSideWidgets();
		void calculateHeaderAreas();

	private:
		RectF mPreviewArea = {};
		Side mPreviewSide = NONE;
		int resizeType[2] = { 0, 0 };

		SideWidgetData mSideWidgets[4];

		std::vector<Widget*> mDockedWidgets;

	private:
		bool mResizing = false;

		Widget* mCenterWidget = nullptr;
		RectF mCenterArea { 0, 0, 10, 10 };

		// Parameters
		const halnf mHandleSplitFactor = 0.3;
		const halnf mHandleFactor = 0.1;

		halnf mSideSizePadding = 150.f;
		halnf mPadding = 4;
		halnf mHeaderSize = 27;
	};
}