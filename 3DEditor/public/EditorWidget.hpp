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

	class EditorWidget : public Widget {
	public:
		EditorWidget(Canvas* canvas, Editor* editor) :
			mViewport(canvas, editor) {
			mEditor = editor;

			this->mChildWidgets.pushBack(&mViewport);
			this->mChildWidgets.pushBack(&mSplitView);
			this->mChildWidgets.pushBack(&mSettingsWidget);

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

			mSettingsWidget.addWidget(&mRenderMenu);
			mSettingsWidget.addWidget(&mNavigationMenu);
		}

		void eventProcess(const Events& events) override {
			mSplitView.setArea(this->mArea);

			mViewport.setArea(mSplitView.getFirst());
			mViewport.mEnable = mSplitView.getFirstEnabled();

			mSettingsWidget.setArea(mSplitView.getSecond());
			mSettingsWidget.mEnable = mSplitView.getSecondEnabled();

			// render settings
			{
				if (mRenderPathTracer.isFired()) {
					mEditor->renderPathFrame();
					mEditor->setRenderType(Editor::RenderType::PATH_TRACER);
				}

				if (mRenderRaster.isFired()) {
					mEditor->setRenderType(Editor::RenderType::RASTER);
				}

				if (mRenderDeNoise.isFired()) {
					mEditor->denoisePathRenderBuffers();
				}
			}

			// navigation
			{
				if (mNavigationOrbit.isFired()) {
					mNavigationType = ORBIT;
				}

				if (mNavigationPan.isFired()) {
					mNavigationType = PAN;
				}

				if (mNavigationZoom.isFired()) {
					mNavigationType = ZOOM;
				}

				if (mNavigationReset.isFired()) {
					mEditor->navigationReset();
				}

				const auto& activeArea = mViewport.mArea;
				if (this->isHolding() && activeArea.isInside(events.getPointer())) {
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
		}

		void eventDraw(Canvas& canvas) override { canvas.rect(this->mArea, mBaseColor); }

		void eventUpdateConfiguration(WidgetManager& wm) override {
			wm.setActiveId("3DEditor");
			mBaseColor = wm.getColor("Base", "Base");
		}

	public:
		Editor* mEditor = nullptr;

		SplitView mSplitView;

		ViewportWidget mViewport;
		ScrollableWindow mSettingsWidget;

		// Controls
		CollapsableMenu mRenderMenu;
		ButtonWidget mRenderPathTracer;
		ButtonWidget mRenderRaster;
		ButtonWidget mRenderDeNoise;

		// Navigation
		enum NavigationType { ORBIT, PAN, ZOOM } mNavigationType = ORBIT;

		CollapsableMenu mNavigationMenu;
		ButtonWidget mNavigationPan;
		ButtonWidget mNavigationOrbit;
		ButtonWidget mNavigationZoom;
		ButtonWidget mNavigationReset;

		RGBA mBaseColor;
	};
}