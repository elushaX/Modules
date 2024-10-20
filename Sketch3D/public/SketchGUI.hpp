#pragma once

#include "Sketch3D.hpp"
#include "Widget.hpp"
#include "DockWidget.hpp"
#include "FloatingWidget.hpp"
#include "ColorPickerWidget.hpp"

namespace tp {

	class SketchRenderWidget : public Widget {
	public:
		SketchRenderWidget(Canvas& canvas, Vec2F renderResolution);
		~SketchRenderWidget() override;

		void draw(Canvas& canvas) override;
		void setProject(Project* project);

	private:
		Renderer mRenderer;
		Project* mProject = nullptr;
		Canvas::ImageHandle mImage;
		Canvas* mCanvas = nullptr;
	};

	class SketchViewportWidget : public Widget {
	public:
		SketchViewportWidget();

		void setProject(Project* project);
		void setColor(const RGBA& color);

		void setRenderWidget(SketchRenderWidget* widget);
		void setToolbarWidget(Widget* widget);

		void process(const EventHandler& events) override;
		[[nodiscard]] bool processesEvents() const override { return true; }

	private:
		Project* mProject = nullptr;

	public:
		Vec2F mActionPosAbsolutePrev = { 0, 0 };
		bool mAction = false;
	};


	class ToolbarWidget : public ScrollableWidget {
	public:
		ToolbarWidget();

	private:
		void addToToolbar(Widget* button);

	public:
		HoverPopupTriggerWidget mFileHover;
		HoverPopupTriggerWidget mPencilHover;
		HoverPopupTriggerWidget mViewHover;
	};

	class SketchGUI : public DockWidget {
	public:
		SketchGUI();
		void createRenderWidget(Canvas* canvas, const Vec2F& renderResolution);
		void setProject(Project* project);
		~SketchGUI() override;

		void process(const EventHandler& events) override;
		void draw(Canvas& canvas) override;

	private:
		Project* mProject = nullptr;

		SketchViewportWidget mViewport;
		ToolbarWidget mToolbar;

		SketchRenderWidget* mRenderWidget = nullptr;

		FloatingMenu mPanel;

		RGBPickerWidget mColorPicker;

		ButtonWidget mToggleSidePanel;

		ButtonWidget mSelectPencil;
		ButtonWidget mSelectEraser;

		SliderWidget mBrushSizeSlider;

		FloatingMenu mControls;

		RGBA mBackgroundColor;
		halnf mRounding = 0;
	};
}
