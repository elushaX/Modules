#pragma once

#include "Sketch3D.hpp"
#include "Widgets.hpp"

namespace tp {

	template <typename Events, typename Canvas>
	class Sketch3DWidget : public Widget<Events, Canvas> {
	public:
		Sketch3DWidget(Canvas& canvas, Vec2F renderResolution) :
			mRenderer(renderResolution) { 
			mImage = canvas.createImageFromTextId(mRenderer.getBuff()->texId(), mRenderer.getBuff()->getSize());
			mCanvas = &canvas;
		}

		~Sketch3DWidget() { 
			mCanvas->deleteImageHandle(mImage);
		}

		void proc(const Events& events, const RectF& areaParent, const RectF& area) override {

			this->mArea = area;
			this->mVisible = area.isOverlap(areaParent);
			if (!this->mVisible) return;

			if (!this->mArea.isInside(events.getPos())) {
				return;
			}

			auto crs = (events.getPos() - this->mArea.pos);
			crs.x /= this->mArea.z;
			crs.y /= this->mArea.w;
			crs = (crs - 0.5) * 2;

			mProject.sample(events.pressure, this->mArea.w / this->mArea.z, crs);

			// mProject.setBackGroundColor();
		}

		void draw(Canvas& canvas) override {
			if (!this->mVisible) return;
			mRenderer.renderToTexture(&mProject, this->mArea.size);
			canvas.drawImage(this->mArea, &mImage, 0, 1, 12);
		}

	private:
		Renderer mRenderer;
		Project mProject;
		Canvas::ImageHandle mImage;
		Canvas* mCanvas = nullptr;
	};
}