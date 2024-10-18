#pragma once

#include "Sketch3D.hpp"
#include "Widget.hpp"
#include "DockWidget.hpp"
#include "FloatingWidget.hpp"

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

		void process(const EventHandler& events) override {
			auto area = getArea().relative();

			Vec2F pointer = events.getPointer();
			auto crs = events.getPointer();

			if (!area.isInside(events.getPointer())) {
				return;
			}

			crs.x /= area.z;
			crs.y /= area.w;
			crs = (crs - 0.5) * 2;

			if (events.isPressed(InputID::MOUSE1)) {
				mAction = true;
			} else if (events.isReleased(InputID::MOUSE1)) {
				mAction = false;
			}

			Vec2F relativePos = ((pointer / area.size) - 0.5f) * 2.f;
			Vec2F relativePosPrev = ((mActionPosAbsolutePrev / area.size) - 0.5f) * 2.f;
			Vec2F relativeDelta = relativePos - relativePosPrev;

			mProject.mCamera.setRatio(area.w / area.z);

			switch (mMode) {
				case Mode::MOVE:
					{
						if (mAction) mProject.mCamera.move(relativePos, relativePosPrev);
						break;
					}
				case Mode::ZOOM:
					{
						halnf factor = pointer.y / mActionPosAbsolutePrev.y;
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
						mProject.sample(events.getPointerPressure(), area.w / area.z, crs);
						break;
					}
				default: break;
			}

			mActionPosAbsolutePrev = pointer;
		}

		void draw(Canvas& canvas) override {
			auto area = getArea().relative();
			mRenderer.renderToTexture(&mProject, area.size);
			canvas.drawImage(area, &mImage, 0, 1, 12);
		}

		void setColor(const RGBA& color) { ((PencilBrush*) mProject.mBrushes.get("pencil"))->mCol = color; }

		[[nodiscard]] bool processesEvents() const override { return true; }

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

	class Sketch3DGUI : public DockWidget {
	public:
		explicit Sketch3DGUI(Canvas& canvas, Vec2F renderResolution) :
			DockWidget(),
			mViewport(canvas, renderResolution) {

			mControls.addToMenu(&mDrawButton);
			mControls.addToMenu(&mMoveButton);
			mControls.addToMenu(&mRotateButton);
			mControls.addToMenu(&mZoomButton);

			mColorPicker.addToMenu(&mRed);
			mColorPicker.addToMenu(&mGreen);
			mColorPicker.addToMenu(&mBlue);

			setCenterWidget(&mViewport);

			mPanel.addToMenu(&mControls);
			mPanel.addToMenu(&mColorPicker);

			//  addChild(&mPanel);
			dockWidget(&mPanel, DockLayout::RIGHT);

			mDrawButton.setAction([this]() { mViewport.mMode = Sketch3DWidget::Mode::DRAW; });
			mMoveButton.setAction([this]() { mViewport.mMode = Sketch3DWidget::Mode::MOVE; });
			mRotateButton.setAction([this]() { mViewport.mMode = Sketch3DWidget::Mode::ROTATE; });
			mZoomButton.setAction([this]() { mViewport.mMode = Sketch3DWidget::Mode::ZOOM; });

			mDrawButton.setText("Draw");
			mMoveButton.setText("Pan");
			mRotateButton.setText("Orbit");
			mZoomButton.setText("Zoom");

			mPanel.setText("Controls");
			mControls.setText("Tools");
			mColorPicker.setText("Color");

			mPanel.setArea({110, 110, 300, 500 });
			mControls.setArea({0, 0, 300, 400 });
			mColorPicker.setArea({0, 0, 300, 300 });

		}

		void process(const EventHandler& events) override {
			DockWidget::process(events);

			mViewport.setColor(RGBA(mRed.val(), mGreen.val(), mBlue.val(), 1.f));
		}

		void draw(Canvas& canvas) override { canvas.rect(getArea().relative(), mBackgroundColor, mRounding); }

	private:
		Sketch3DWidget mViewport;

		FloatingMenu mPanel;

		FloatingMenu mControls;
		ButtonWidget mDrawButton;
		ButtonWidget mMoveButton;
		ButtonWidget mRotateButton;
		ButtonWidget mZoomButton;

		FloatingMenu mColorPicker;
		SliderWidget mRed;
		SliderWidget mGreen;
		SliderWidget mBlue;

		RGBA mBackgroundColor;
		halnf mRounding = 0;
	};
}