#include "Widgets.hpp"
#include "Render.hpp"

namespace tp {
	template <typename Events, typename Canvas>
	class ViewportWidget : public Widget<Events, Canvas> {
	public:
		explicit ViewportWidget(Canvas* canvas, Scene* geometry, Vec2F renderResolution) :
			mRender(renderResolution) {
			mGeometry = geometry;
			this->createConfig("ViewportWidget");
			mImage = canvas->createImageFromTextId(mRender.getRenderBuffer(), mRender.getBufferSize());
			mCanvas = canvas;
		}

		~ViewportWidget() { mCanvas->deleteImageHandle(mImage); }

		void proc(const Events& events, const RectF& areaParent, const RectF& aArea) override {
			this->mArea = aArea;
			this->mVisible = areaParent.isOverlap(aArea);
			if (!this->mVisible) return;

			mGeometry->mCamera.rotate(0.01f, 0.0);
		}

		void draw(Canvas& canvas) override {
			if (!this->mVisible) return;

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
			this->createConfig("EditorWidget");
			this->addColor("Base", "Base");
		}

		void proc(const Events& events, const RectF& areaParent, const RectF& aArea) override {
			this->mArea = aArea;
			this->mVisible = areaParent.isOverlap(aArea);
			if (!this->mVisible) return;

			mSplitView.proc(events, aArea, aArea);
			mViewport.proc(events, aArea, mSplitView.getFirst());
		}

		void draw(Canvas& canvas) override {
			if (!this->mVisible) return;

			canvas.rect(this->mArea, this->getColor("Base"));
			mSplitView.draw(canvas);
			mViewport.draw(canvas);
		}

	public:
		ViewportWidget<Events, Canvas> mViewport;
		SplitView<Events, Canvas> mSplitView;
	};
}