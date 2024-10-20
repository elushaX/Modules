#include "DockLayout.hpp"

namespace tp {
	class DockWidget : public Widget {
	public:
		DockWidget();

	public:
		void process(const EventHandler& events) override;
		void draw(Canvas& canvas) override;
		void drawOverlay(Canvas& canvas) override;

		[[nodiscard]] bool propagateEventsToChildren() const override;
		[[nodiscard]] bool needsNextFrame() const override;
		[[nodiscard]]bool processesEvents() const override { return true; }

		void drawSide(DockLayout::Side side, Canvas& canvas);

	public:
		DockLayout::Side getSide(Widget* widget) { return layout()->getSideFromWidget(widget); }

		void setCenterWidget(Widget* widget);

		void dockWidget(Widget* widget, DockLayout::Side side);
		void undockWidget(DockLayout::Side side, bool restoreArea = true);
		void toggleWidgetVisibility(DockLayout::Side side);

	private:
		DockLayout* layout() { return dynamic_cast<DockLayout*>(mLayout); }
		const DockLayout* layout() const { return dynamic_cast<const DockLayout*>(mLayout); }

	private:
		Widget* mPreviewWidget = nullptr;

	private:
		halnf mRounding = 10;
		halnf mPadding = 4;

		RGBA mResizeHandleColorHovered = RGBA(0.3, 0.3, 0.3, 1);
		RGBA mResizeHandleColorActive = RGBA(0.6, 0.6, 0.6, 1);
		RGBA mBackgroundColor = RGBA(0, 0, 0, 1);
		RGBA mPreviewColor = RGBA(0.6, 0.6, 0.6, 0.2);
	};
}