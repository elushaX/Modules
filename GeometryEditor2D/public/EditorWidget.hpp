#include "Widgets.hpp"
#include "Render.hpp"

namespace tp {
	template <typename Events, typename Canvas>
	class ViewportWidget : public Widget<Events, Canvas> {
	public:
		explicit ViewportWidget(Canvas* canvas, Geometry* geometry, Vec2F renderResolution) :
			mRender(renderResolution) {
			this->createConfig("ViewportWidget");
			mImage = canvas->createImageFromTextId(mRender.getRenderBuffer(), mRender.getBufferSize());
			mCanvas = canvas;
		}

		~ViewportWidget() { mCanvas->deleteImageHandle(mImage); }

		void proc(const Events& events, const RectF& areaParent, const RectF& aArea) override {
			this->mArea = aArea;
			this->mVisible = areaParent.isOverlap(aArea);
			if (!this->mVisible) return;

			mRender.render(*mGeometry, this->mArea.size);
		}

		void draw(Canvas& canvas) override {
			if (!this->mVisible) return;

			canvas.drawImage(this->mArea, &mImage);
		}

	public:
		Render mRender;
		Geometry* mGeometry = nullptr;
		Canvas* mCanvas = nullptr;
		Canvas::ImageHandle mImage;
	};

	template <typename Events, typename Canvas>
	class EditorWidget : public Widget<Events, Canvas> {
	public:
		EditorWidget(Canvas* canvas, Geometry* geometry, Vec2F renderResolution) :
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