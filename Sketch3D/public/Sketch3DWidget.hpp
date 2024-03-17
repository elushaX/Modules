#pragma once

#include "Sketch3D.hpp"
#include "Widgets.hpp"

namespace tp {

	template <typename Events, typename Canvas>
	class Sketch3DWidget : public Widget<Events, Canvas> {
	public:

		Canvas* mCanvas = nullptr;

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

			mProject.mCamera.setRatio(this->mArea.w / this->mArea.z);

			// mProject.mBackgroundColor = col_obj_bg->mCol;

			if (!this->mArea.isInside(events.getPos())) {
				return;
			}

			halnf pressure = 0.f;

			auto idx = mProject.mBrushes.presents(mProject.mActiveBrush);
			if (idx) {
				auto brush = mProject.mBrushes.getSlotVal(idx);
				auto crs = (events.getPos() - this->mArea.pos);
				crs.x /= this->mArea.z;
				crs.y /= this->mArea.w;
				crs = (crs - 0.5) * 2;

				if (brush->mType == "pencil") {
					((tp::PencilBrush*) brush)->mCol = RGBA(1.f);
				}

				brush->sample(&mProject, crs, pressure);
			}
		}

		void draw(Canvas& canvas) override {
			
			mRenderer.setViewport({ 0, 0, this->mArea.z, this->mArea.w });
			mRenderer.setClearCol(mProject.mBackgroundColor);
			mRenderer.renderBegin();

			for (auto lay : mProject.mLayers) {
				if (lay.data()->enabled) {
					for (auto str : lay.data()->strokes) {
						mRenderer.drawStroke(str.data(), &mProject.mCamera);
					}
				}
			}

			auto idx = mProject.mBrushes.presents(mProject.mActiveBrush);
			if (idx) {
				auto brush = mProject.mBrushes.getSlotVal(idx);
				brush->draw(&mRenderer, &mProject.mCamera);
			}

			mRenderer.renderEnd();

			canvas.drawImage(this->mArea, &mImage, 0, 1, 12);
		}

	private:
		Project mProject;
		Renderer mRenderer;
		Canvas::ImageHandle mImage;
	};
}