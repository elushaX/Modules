#include "Widget.hpp"
#include "DockWidget.hpp"

#include "Editor.hpp"
#include "FloatingWidget.hpp"

namespace tp {

	class ViewportWidget : public Widget {
	public:
		explicit ViewportWidget(Canvas* canvas, Editor* editor) {
			mEditor = editor;
			mImage = canvas->createImageFromTextId(mEditor->getViewportTexID(), { 0, 0 });
			mCanvas = canvas;
		}

		~ViewportWidget() override { mCanvas->deleteImageHandle(mImage); }

		void process(const EventHandler& events) override {
			mEditor->setViewportSize(getAreaT().size);
		}

		void draw(Canvas& canvas) override {
			mEditor->renderViewport();

			canvas.updateTextureID(mImage, mEditor->getViewportTexID());
			canvas.drawImage(getArea().relative(), &mImage, PI);
		}

	public:
		Editor* mEditor;

		Canvas* mCanvas = nullptr;
		Canvas::ImageHandle mImage;
	};

	class EditorWidget : public DockWidget {
	public:
		EditorWidget(Canvas* canvas, Editor* editor) :
			mViewport(canvas, editor) {
			mEditor = editor;

			mPanel.setText("Controls");
			mPanel.addToMenu(&mNavigationMenu);
			mPanel.addToMenu(&mRenderMenu);

			dockWidget(&mPanel, DockLayout::RIGHT);
			setCenterWidget(&mViewport);

			// Render
			{
				mRenderPathTracer.setText("Render with Path Tracer");
				mRenderRaster.setText("Render with Raster");
				mRenderDeNoise.setText("Denoise (IntelOpenImage)");

				mRenderMenu.addToMenu(&mRenderPathTracer);
				mRenderMenu.addToMenu(&mRenderRaster);
				mRenderMenu.addToMenu(&mRenderDeNoise);

				mRenderMenu.setText("Render");
			}

			// Navigation
			{
				mNavigationPan.setText("Pan");
				mNavigationOrbit.setText("Orbit");
				mNavigationZoom.setText("Zoom");
				mNavigationReset.setText("Reset");
				mNavigationSelect.setText("Select");

				mNavigationMenu.addToMenu(&mNavigationPan);
				mNavigationMenu.addToMenu(&mNavigationOrbit);
				mNavigationMenu.addToMenu(&mNavigationZoom);
				mNavigationMenu.addToMenu(&mNavigationReset);
				mNavigationMenu.addToMenu(&mNavigationSelect);

				mNavigationMenu.setText("Navigation");
			}

			mRenderPathTracer.setAction([this]() {
				mEditor->renderPathFrame();
				mEditor->setRenderType(Editor::RenderType::PATH_TRACER);
			});

			mRenderRaster.setAction( [this]() { mEditor->setRenderType(Editor::RenderType::RASTER); });
			mRenderDeNoise.setAction( [this]() { mEditor->denoisePathRenderBuffers(); });
			mNavigationOrbit.setAction( [this]() { mNavigationType = ORBIT; });
			mNavigationPan.setAction( [this]() { mNavigationType = PAN; });
			mNavigationZoom.setAction( [this](){ mNavigationType = ZOOM; });
			mNavigationSelect.setAction( [this](){ mNavigationType = SELECT; });
			mNavigationReset.setAction( [this]() { mEditor->navigationReset(); });
		}

		void process(const EventHandler& events) override {
			DockWidget::process(events);

			if (auto obj = mEditor->getActiveObject()) { // dummy rotate active object
				auto rotator = obj->mTopology.Basis.rotatorDir({1, 1, 1}, 0.1);
				obj->mTopology.Basis = rotator * obj->mTopology.Basis;
			}

			mEditor->getScene()->updateCache();

			const auto& activeArea = mViewport.getArea();

			auto pointer = events.getPointer();
			auto pointerPrev = events.getPointerPrev();
			auto pointerRelative = (((pointer - activeArea.pos) / activeArea.size) - 0.5f) * 2;

			if (activeArea.isInside(pointer) && events.isDown(InputID::MOUSE1)) {
				switch (mNavigationType) {
					case ORBIT: mEditor->navigationOrbit(events.getPointerDelta() / activeArea.size * 3); break;

					case PAN:
						{
							auto prevPointerRelative = (((pointerPrev - activeArea.pos) / activeArea.size) - 0.5f) * 2;
							mEditor->navigationPan(prevPointerRelative, pointerRelative);
						}
						break;

					case ZOOM: mEditor->navigationZoom(1 + (events.getPointerDelta().y / activeArea.size.y)); break;
					case SELECT: mEditor->selectObject(pointerRelative * -1); break;
				}
			}
		}

		void draw(Canvas& canvas) override { canvas.rect(getArea().relative(), mBaseColor); }

	public:
		Editor* mEditor = nullptr;

		ViewportWidget mViewport;

		FloatingMenu mPanel;

		// Controls
		FloatingMenu mRenderMenu;
		ButtonWidget mRenderPathTracer;
		ButtonWidget mRenderRaster;
		ButtonWidget mRenderDeNoise;

		// Navigation
		enum NavigationType { SELECT, ORBIT, PAN, ZOOM } mNavigationType = ORBIT;

		FloatingMenu mNavigationMenu;
		ButtonWidget mNavigationPan;
		ButtonWidget mNavigationOrbit;
		ButtonWidget mNavigationZoom;
		ButtonWidget mNavigationReset;
		ButtonWidget mNavigationSelect;

		RGBA mBaseColor;
	};
}