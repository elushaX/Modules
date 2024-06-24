#pragma once

#include "Sketch3D.hpp"
#include "Widgets.hpp"

namespace tp {

	class Sketch3DWidget : public Widget {
	public:
		Sketch3DWidget(Canvas& canvas, Vec2F renderResolution) :
			mRenderer(renderResolution) {
			mImage = canvas.createImageFromTextId(mRenderer.getBuff()->texId(), mRenderer.getBuff()->getSize());
			mCanvas = &canvas;

			mProject.mBackgroundColor = { 0.13f, 0.13f, 0.13f, 1.f };
		}

		~Sketch3DWidget() { mCanvas->deleteImageHandle(mImage); }

		void eventProcess(const Events& events) override {
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
				case Mode::MOVE:
					{
						if (mAction) mProject.mCamera.move(relativePos, relativePosPrev);
						break;
					}
				case Mode::ZOOM:
					{
						halnf factor = absolutePos.y / mActionPosAbsolutePrev.y;
						if (mAction) mProject.mCamera.zoom(factor);
						break;
					}
				case Mode::ROTATE:
					{
						if (mAction) mProject.mCamera.rotate(-relativeDelta.x * halnf(PI), -relativeDelta.y * halnf(PI));
						break;
					}
				case Mode::DRAW:
					{
						mProject.sample(events.getPointerPressure(), this->mArea.w / this->mArea.z, crs);
						break;
					}
				default: break;
			}

			mActionPosAbsolutePrev = absolutePos;
		}

		void eventDraw(Canvas& canvas) override {
			mRenderer.renderToTexture(&mProject, this->mArea.size);
			canvas.drawImage(this->mArea, &mImage, 0, 1, 12);
		}

		void setColor(const RGBA& color) { ((PencilBrush*) mProject.mBrushes.get("pencil"))->mCol = color; }

	public:
		enum class Mode { MOVE, ROTATE, ZOOM, DRAW, NONE } mMode = Mode::NONE;

		Vec2F mActionPosAbsolutePrev = { 0, 0 };
		bool mAction = false;

	private:
		Renderer mRenderer;
		Project mProject;
		Canvas::ImageHandle mImage;
		Canvas* mCanvas = nullptr;
	};

	
	class Sketch3DGUI : public Widget {
	public:
		Sketch3DGUI(Canvas& canvas, Vec2F renderResolution) :
			mViewport(canvas, renderResolution) {
			mDrawButton = new ButtonWidget("Draw", { 0, 0, 100, 30 });
			mMoveButton = new ButtonWidget("Pan View", { 0, 0, 100, 30 });
			mRotateButton = new ButtonWidget("Rotate view", { 0, 0, 100, 30 });
			mZoomButton = new ButtonWidget("Zoom view", { 0, 0, 100, 30 });

			mOptions.addWidget(mDrawButton);
			mOptions.addWidget(mMoveButton);
			mOptions.addWidget(mRotateButton);
			mOptions.addWidget(mZoomButton);

			// add color sliders
			mRed = new NamedSliderWidget("Red");
			mGreen = new NamedSliderWidget("Green");
			mBlue = new NamedSliderWidget("Blue");

			mOptions.addWidget(mRed);
			mOptions.addWidget(mGreen);
			mOptions.addWidget(mBlue);

			this->mChildWidgets.pushBack(&mViewport);
			this->mChildWidgets.pushBack(&mSplitView);
			this->mChildWidgets.pushBack(&mOptions);
		}

		~Sketch3DGUI() {
			for (auto item : mOptions.getContent()) {
				delete item.data();
			}
		}

		void eventProcess(const Events& events) override {
			mSplitView.setArea(this->mArea);
			mViewport.setArea(mSplitView.getFirst());
			mOptions.setArea(mSplitView.getSecond());

			if (mDrawButton->isFired()) {
				mViewport.mMode = Sketch3DWidget::Mode::DRAW;
			} else if (mMoveButton->isFired()) {
				mViewport.mMode = Sketch3DWidget::Mode::MOVE;
			} else if (mRotateButton->isFired()) {
				mViewport.mMode = Sketch3DWidget::Mode::ROTATE;
			} else if (mZoomButton->isFired()) {
				mViewport.mMode = Sketch3DWidget::Mode::ZOOM;
			}

			mViewport.setColor(RGBA(mRed->mSlider.mFactor, mGreen->mSlider.mFactor, mBlue->mSlider.mFactor, 1.f));
		}

		void eventDraw(Canvas& canvas) override { canvas.rect(this->mArea, mBackgroundColor, mRounding); }

		void eventUpdateConfiguration(WidgetManager& wm) override {
			wm.setActiveId("Sketch3DGui");

			mBackgroundColor = wm.getColor("Background", "Background");
			mRounding = wm.getNumber("Rounding", "Rounding");
		}

	private:
		Sketch3DWidget mViewport;
		SplitView mSplitView;
		ScrollableWindow mOptions;

		ButtonWidget* mDrawButton = nullptr;
		ButtonWidget* mMoveButton = nullptr;
		ButtonWidget* mRotateButton = nullptr;
		ButtonWidget* mZoomButton = nullptr;

		NamedSliderWidget* mRed = nullptr;
		NamedSliderWidget* mGreen = nullptr;
		NamedSliderWidget* mBlue = nullptr;

		RGBA mBackgroundColor;
		halnf mRounding = 0;
	};
}