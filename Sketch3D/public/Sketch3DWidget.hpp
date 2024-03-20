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

			mProject.mBackgroundColor = { 0.13f, 0.13f, 0.13f, 1.f };
		}

		~Sketch3DWidget() { 
			mCanvas->deleteImageHandle(mImage);
		}

		void proc(const Events& events, const RectF& areaParent, const RectF& area) override {

			this->mArea = area;
			this->mVisible = area.isOverlap(areaParent);
			if (!this->mVisible) return;

			if (!this->mArea.isInside(events.getPointer())) {
				return;
			}

			auto crs = (events.getPointer() - this->mArea.pos);
			crs.x /= this->mArea.z;
			crs.y /= this->mArea.w;
			crs = (crs - 0.5) * 2;

			// TODO : make better api for events
			Vec2F absolutePos = events.getPointer() - this->mArea.pos;

			if (events.isPressed(InputID::MOUSE1)) {
				mAction = true;
			} else if (events.isReleased(InputID::MOUSE1)) {
				mAction = false;
			}

			Vec2F relativePos = ((absolutePos / this->mArea.size) - 0.5f) * 2.f;
			Vec2F relativePosPrev = ((mActionPosAbsolutePrev / this->mArea.size) - 0.5f) * 2.f;
			Vec2F relativeDelta = relativePos - relativePosPrev;

			mProject.mCamera.setRatio(this->mArea.w / this->mArea.z);

			switch (mMode) {
				case Mode::MOVE: {
						if (mAction) mProject.mCamera.move(relativePos, relativePosPrev);
						break;
					}
				case Mode::ZOOM: {
						halnf factor = absolutePos.y / mActionPosAbsolutePrev.y;
						if (mAction) mProject.mCamera.zoom(factor);
						break;
					}
				case Mode::ROTATE: {
						if (mAction) mProject.mCamera.rotate(-relativeDelta.x * halnf(PI), -relativeDelta.y * halnf(PI));
						break;
					}
				case Mode::DRAW: {
						mProject.sample(events.getPointerPressure(), this->mArea.w / this->mArea.z, crs);
						break;
					}
				default: break;
			}

			mActionPosAbsolutePrev = absolutePos;
		}

		void draw(Canvas& canvas) override {
			if (!this->mVisible) return;
			mRenderer.renderToTexture(&mProject, this->mArea.size);
			canvas.drawImage(this->mArea, &mImage, 0, 1, 12);
		}

	public:
		enum class Mode {
			MOVE, ROTATE, ZOOM, DRAW, NONE
		} mMode = Mode::NONE;

		Vec2F mActionPosAbsolutePrev = { 0, 0 };
		bool mAction = false;

	private:
		Renderer mRenderer;
		Project mProject;
		Canvas::ImageHandle mImage;
		Canvas* mCanvas = nullptr;
	};

	template <typename Events, typename Canvas>
	class Sketch3DGUI : public Widget<Events, Canvas> {
	public:
		Sketch3DGUI(Canvas& canvas, Vec2F renderResolution) : mViewport(canvas, renderResolution) {
			this->createConfig("Sketch3D");

			this->addColor("Background", "Background");
			this->addValue("Rounding", "Rounding");

			mDrawButton = new ButtonWidget<Events, Canvas>("Draw", { 0, 0, 100, 30 });
			mMoveButton = new ButtonWidget<Events, Canvas>("Pan View", { 0, 0, 100, 30 });
			mRotateButton = new ButtonWidget<Events, Canvas>("Rotate view", { 0, 0, 100, 30 });
			mZoomButton = new ButtonWidget<Events, Canvas>("Zoom view", { 0, 0, 100, 30 });

			mOptions.mContents.append(mDrawButton);
			mOptions.mContents.append(mMoveButton);
			mOptions.mContents.append(mRotateButton);
			mOptions.mContents.append(mZoomButton);
		}

		void proc(const Events& events, const RectF& areaParent, const RectF& area) override {
			this->mArea = area;
			this->mVisible = area.isOverlap(areaParent);
			if (!this->mVisible) return;

			mSplitView.proc(events, this->mArea, this->mArea);
			mViewport.proc(events, this->mArea, mSplitView.getFirst());
			mOptions.proc(events, this->mArea, mSplitView.getSecond());

			if (mDrawButton->mIsPressed) {
				mViewport.mMode = Sketch3DWidget<Events, Canvas>::Mode::DRAW;
			} else if (mMoveButton->mIsPressed) {
				mViewport.mMode = Sketch3DWidget<Events, Canvas>::Mode::MOVE;
			} else if (mRotateButton->mIsPressed) {
				mViewport.mMode = Sketch3DWidget<Events, Canvas>::Mode::ROTATE;
			} else if (mZoomButton->mIsPressed) {
				mViewport.mMode = Sketch3DWidget<Events, Canvas>::Mode::ZOOM;
			}
		}

		void draw(Canvas& canvas) override {
			if (!this->mVisible) return;

			canvas.rect(this->mArea, this->getColor("Background"), this->getValue("Rounding"));

			mSplitView.draw(canvas);
			mViewport.draw(canvas);
			mOptions.draw(canvas);
		}

	private:
		Sketch3DWidget<Events, Canvas> mViewport;
		SplitView<Events, Canvas> mSplitView;
		ScrollableWindow<Events, Canvas> mOptions;

		ButtonWidget<Events, Canvas>* mDrawButton = nullptr;
		ButtonWidget<Events, Canvas>* mMoveButton = nullptr;
		ButtonWidget<Events, Canvas>* mRotateButton = nullptr;
		ButtonWidget<Events, Canvas>* mZoomButton = nullptr;
	};
}