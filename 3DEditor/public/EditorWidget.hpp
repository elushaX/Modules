#include "Widgets.hpp"

#include "Editor.hpp"

namespace tp {

	template <typename Events, typename Canvas>
	class ViewportWidget : public Widget<Events, Canvas> {
	public:
		explicit ViewportWidget(Canvas* canvas, Editor* editor) {
			mEditor = editor;
			mImage = canvas->createImageFromTextId(mEditor->getViewportTexID(), { 0, 0 });
			mCanvas = canvas;
		}

		~ViewportWidget() { mCanvas->deleteImageHandle(mImage); }

		void eventProcess(const Events& events) override {
			mEditor->setViewportSize(this->mArea.size);
		}

		void eventDraw(Canvas& canvas) override {
			mEditor->renderViewport();
			canvas.drawImage(this->mArea, &mImage, PI);
		}

	public:
		Editor* mEditor;

		Canvas* mCanvas = nullptr;
		Canvas::ImageHandle mImage;
	};

	template <typename Events, typename Canvas>
	class EditorWidget : public Widget<Events, Canvas> {
	public:
		EditorWidget(Canvas* canvas, Editor* editor) :
			mViewport(canvas, editor)
		{
			this->mChildWidgets.pushBack(&mViewport);
			this->mChildWidgets.pushBack(&mSplitView);
		}

		void eventProcess(const Events& events) override {
			mSplitView.setArea(this->mArea);
			mViewport.setArea(mSplitView.getFirst());
		}

		void eventDraw(Canvas& canvas) override { canvas.rect(this->mArea, mBaseColor); }

		void eventUpdateConfiguration(WidgetManager& wm) override {
			wm.setActiveId("3DEditor");
			mBaseColor = wm.getColor("Base", "Base");
		}

	public:
		ViewportWidget<Events, Canvas> mViewport;
		SplitView<Events, Canvas> mSplitView;

		RGBA mBaseColor;
	};
}