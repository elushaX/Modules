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

			canvas.updateTextureID(mImage, mEditor->getViewportTexID());
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
			mEditor = editor;

			this->mChildWidgets.pushBack(&mViewport);
			this->mChildWidgets.pushBack(&mSplitView);
			this->mChildWidgets.pushBack(&mSettingsWidget);

			mRenderPathTracer.mLabel.mLabel = "Render with Path Tracer";
			mRenderRaster.mLabel.mLabel = "Render with Raster";
			mDenoiseButton.mLabel.mLabel = "Denoise (IntelOpenImage)";

			mSettingsWidget.addWidget(&mRenderPathTracer);
			mSettingsWidget.addWidget(&mRenderRaster);
			mSettingsWidget.addWidget(&mDenoiseButton);
		}

		void eventProcess(const Events& events) override {
			mSplitView.setArea(this->mArea);

			mViewport.setArea(mSplitView.getFirst());
			mSettingsWidget.setArea(mSplitView.getSecond());

			if (mRenderPathTracer.isFired()) {
				mEditor->renderPathFrame();
				mEditor->setRenderType(Editor::RenderType::PATH_TRACER);
			}

			if (mRenderRaster.isFired()) {
				mEditor->setRenderType(Editor::RenderType::RASTER);
			}

			if (mDenoiseButton.isFired()) {
				mEditor->denoisePathRenderBuffers();
			}
		}

		void eventDraw(Canvas& canvas) override {
			canvas.rect(this->mArea, mBaseColor);
		}

		void eventUpdateConfiguration(WidgetManager& wm) override {
			wm.setActiveId("3DEditor");
			mBaseColor = wm.getColor("Base", "Base");
		}

	public:
		Editor* mEditor = nullptr;

		SplitView<Events, Canvas> mSplitView;

		ViewportWidget<Events, Canvas> mViewport;

		// Controls
		ScrollableWindow<Events, Canvas> mSettingsWidget;
		ButtonWidget<Events, Canvas> mRenderPathTracer;
		ButtonWidget<Events, Canvas> mRenderRaster;
		ButtonWidget<Events, Canvas> mDenoiseButton;

		RGBA mBaseColor;
	};
}