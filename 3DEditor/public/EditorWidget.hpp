#include "Widgets.hpp"

#include "Editor.hpp"

namespace tp {

	class ViewportWidget : public Widget {
	public:
		explicit ViewportWidget(Canvas* canvas, Editor* editor) {
			mEditor = editor;
			mImage = canvas->createImageFromTextId(mEditor->getViewportTexID(), { 0, 0 });
			mCanvas = canvas;
		}

		~ViewportWidget() { mCanvas->deleteImageHandle(mImage); }

		void eventProcess(const Events& events) override { mEditor->setViewportSize(this->mArea.size); }

		void eventDraw(Canvas& canvas) override {
			mEditor->renderViewport();

			canvas.updateTextureID(mImage, mEditor->getViewportTexID());
			canvas.drawImage(this->mArea, &mImage, PI);
		}

	public:
		Editor* mEditor;

		Canvas* mCanvas = nullptr;
		Canvas::ImageHandle mImage;
	};

	class EditorWidget : public WorkspaceWidget {
	public:
		EditorWidget(Canvas* canvas, Editor* editor) :
			mViewport(canvas, editor) {
			mEditor = editor;

			mDockSpace.addSideWidget(&mNavigationMenu, DockWidget::RIGHT);
			mDockSpace.addSideWidget(&mRenderMenu, DockWidget::LEFT);
			mDockSpace.setCenterWidget(&mViewport);

			// Render
			{
				mRenderPathTracer.setLabel("Render with Path Tracer");
				mRenderRaster.setLabel("Render with Raster");
				mRenderDeNoise.setLabel("Denoise (IntelOpenImage)");

				mRenderMenu.addWidgetToMenu(&mRenderPathTracer);
				mRenderMenu.addWidgetToMenu(&mRenderRaster);
				mRenderMenu.addWidgetToMenu(&mRenderDeNoise);

				mRenderMenu.setLabel("Render");
			}

			// Navigation
			{
				mNavigationPan.setLabel("Pan");
				mNavigationOrbit.setLabel("Orbit");
				mNavigationZoom.setLabel("Zoom");
				mNavigationReset.setLabel("Reset");

				mNavigationMenu.addWidgetToMenu(&mNavigationPan);
				mNavigationMenu.addWidgetToMenu(&mNavigationOrbit);
				mNavigationMenu.addWidgetToMenu(&mNavigationZoom);
				mNavigationMenu.addWidgetToMenu(&mNavigationReset);

				mNavigationMenu.setLabel("Navigation");
			}

			mRenderPathTracer.mCallback = [this]() {
				mEditor->renderPathFrame();
				mEditor->setRenderType(Editor::RenderType::PATH_TRACER);
			};

			mRenderRaster.mCallback = [this]() { mEditor->setRenderType(Editor::RenderType::RASTER); };
			mRenderDeNoise.mCallback = [this]() { mEditor->denoisePathRenderBuffers(); };
			mNavigationOrbit.mCallback = [this]() { mNavigationType = ORBIT; };
			mNavigationPan.mCallback = [this]() { mNavigationType = PAN; };
			mNavigationZoom.mCallback = [this](){ mNavigationType = ZOOM; };
			mNavigationReset.mCallback = [this]() { mEditor->navigationReset(); };
		}

		void eventProcess(const Events& events) override {
			// navigation
			{
				const auto& activeArea = mViewport.mArea;
				if (mViewport.isHolding()) {
					switch (mNavigationType) {
						case ORBIT: mEditor->navigationOrbit(events.getPointerDelta() / activeArea.size * 3); break;

						case PAN:
							{
								auto pointer = (((events.getPointer() - activeArea.pos) / activeArea.size) - 0.5f) * 2;
								auto prevPointer = (((events.getPointerPrev() - activeArea.pos) / activeArea.size) - 0.5f) * 2;
								mEditor->navigationPan(prevPointer, pointer);
							}
							break;

						case ZOOM: mEditor->navigationZoom(1 + (events.getPointerDelta().y / activeArea.size.y)); break;
					}
				}
			}

			WorkspaceWidget::eventProcess(events);
		}

		void eventDraw(Canvas& canvas) override { canvas.rect(this->mArea, mBaseColor); }

		void eventUpdateConfiguration(WidgetManager& wm) override {
			wm.setActiveId("3DEditor");
			mBaseColor = wm.getColor("Base", "Base");
		}

	public:
		Editor* mEditor = nullptr;

		// SplitView mSplitView;

		ViewportWidget mViewport;
		// ScrollableWindow mSettingsWidget;

		// Controls
		FloatingWidget mRenderMenu;
		ButtonWidget mRenderPathTracer;
		ButtonWidget mRenderRaster;
		ButtonWidget mRenderDeNoise;

		// Navigation
		enum NavigationType { ORBIT, PAN, ZOOM } mNavigationType = ORBIT;

		FloatingWidget mNavigationMenu;
		ButtonWidget mNavigationPan;
		ButtonWidget mNavigationOrbit;
		ButtonWidget mNavigationZoom;
		ButtonWidget mNavigationReset;

		RGBA mBaseColor;
	};
}