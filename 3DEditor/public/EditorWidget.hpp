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
			mEditor->setViewportSize(getArea().size);
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

				mNavigationMenu.addToMenu(&mNavigationPan);
				mNavigationMenu.addToMenu(&mNavigationOrbit);
				mNavigationMenu.addToMenu(&mNavigationZoom);
				mNavigationMenu.addToMenu(&mNavigationReset);

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
			mNavigationReset.setAction( [this]() { mEditor->navigationReset(); });
		}

		void process(const EventHandler& events) override {
			DockWidget::process(events);

			auto pointer = events.getPointer();
			auto pointerPrev = events.getPointerPrev();

			const auto& activeArea = mViewport.getArea();

			if (activeArea.isInside(pointer) && events.isDown(InputID::MOUSE1)) {
				switch (mNavigationType) {
					case ORBIT: mEditor->navigationOrbit(events.getPointerDelta() / activeArea.size * 3); break;

					case PAN:
						{
							auto pointerRelative = (((pointer - activeArea.pos) / activeArea.size) - 0.5f) * 2;
							auto prevPointerRelative = (((pointerPrev - activeArea.pos) / activeArea.size) - 0.5f) * 2;
							mEditor->navigationPan(prevPointerRelative, pointerRelative);
						}
						break;

					case ZOOM: mEditor->navigationZoom(1 + (events.getPointerDelta().y / activeArea.size.y)); break;
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
		enum NavigationType { ORBIT, PAN, ZOOM } mNavigationType = ORBIT;

		FloatingMenu mNavigationMenu;
		ButtonWidget mNavigationPan;
		ButtonWidget mNavigationOrbit;
		ButtonWidget mNavigationZoom;
		ButtonWidget mNavigationReset;

		RGBA mBaseColor;
	};
}