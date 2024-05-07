#include "Widgets.hpp"
#include "Render.hpp"

namespace tp {

	template <typename Events, typename Canvas>
	class ViewportWidget : public Widget<Events, Canvas> {
	public:
		explicit ViewportWidget(Canvas* canvas, Scene* geometry, Vec2F renderResolution) :
			mRender(renderResolution) {

			mImage = canvas->createImageFromTextId(mRender.getRenderBuffer(), mRender.getBufferSize());
			mGeometry = geometry;
			mCanvas = canvas;
		}

		~ViewportWidget() { mCanvas->deleteImageHandle(mImage); }

		void eventProcess(const Events& events) override { mGeometry->mCamera.rotate(0.01f, 0.0); }

		void eventDraw(Canvas& canvas) override {
			mRender.render(*mGeometry, this->mArea.size);
			canvas.drawImage(this->mArea, &mImage, PI);
		}

	public:
		Render mRender;
		Scene* mGeometry = nullptr;
		Canvas* mCanvas = nullptr;
		Canvas::ImageHandle mImage;
	};

	template <typename Events, typename Canvas>
	class EditorWidget : public Widget<Events, Canvas> {
	public:
		EditorWidget(Canvas* canvas, Scene* geometry, Vec2F renderResolution) :
			mViewport(canvas, geometry, renderResolution) {

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