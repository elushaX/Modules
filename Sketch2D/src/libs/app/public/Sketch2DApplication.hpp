#pragma once

#include "CanvasPreviewImages.hpp"
#include "CanvasThumbnails.hpp"
#include "CanvasViewport.hpp"
#include "IconSet.hpp"
#include "Sketch2DUI.hpp"
#include "TileRenderer.hpp"
#include "StudioUI.hpp"
#include "Updates.hpp"
#include "WidgetApplication.hpp"

#include <memory>

namespace tp::sketch2d {

	struct Sketch2DOptions {
		AppStorage storage = SketchLibrary::applicationStorage();
		bool autosave = true;
		WindowIdentity window = WindowIdentity::generic();

		[[nodiscard]] static Sketch2DOptions forTesting(const std::string& storageRoot);
	};

	class Sketch2DApplication : public ui::WidgetApplication {
	public:
		explicit Sketch2DApplication(Sketch2DOptions options = {});
		~Sketch2DApplication() override;

		[[nodiscard]] SessionRecorder& sessionRecorder() { return debugTools().recorder(); }

		[[nodiscard]] Document& document() { return mDocument; }
		[[nodiscard]] Brush& brush() { return mBrush; }
		[[nodiscard]] Sketch2DRoot& root() { return mRoot; }
		[[nodiscard]] TileRenderer& renderer() { return mRenderer; }
		[[nodiscard]] StudioRoot* studioRoot() { return mStudioRoot.get(); }
		void checkForUpdates(UpdateBackend& backend);
		[[nodiscard]] AppStorage persistentAppStorage() const override { return mOptions.storage; }

	protected:
		void processFrame(EventHandler* eventHandler, halnf deltaTime) override;
		[[nodiscard]] HangSettings hangSettings() const override;

	private:
		Sketch2DOptions mOptions;
		bool mWasFocused = true;

		Document mDocument;
		Brush mBrush;
		IconSet mIcons;
		TileRenderer mRenderer;
		CanvasViewport mViewport;
		CanvasThumbnails mThumbnails;
		CanvasPreviewImages mPreviews{ canvas() };
		Sketch2DRoot mRoot;
		std::unique_ptr<StudioRoot> mStudioRoot;
		std::unique_ptr<UpdateManager> mUpdates;
	};
}
