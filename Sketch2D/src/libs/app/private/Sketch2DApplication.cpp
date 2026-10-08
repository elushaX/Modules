#include "Sketch2DApplication.hpp"
#include "ThreadPlacement.hpp"

#include <thread>

namespace tp::sketch2d {

	namespace {
		Brush& initialBrush(Brush& brush) {
			brush.apply(*findBrushPreset(BrushType::Round));
			return brush;
		}
	}

	Sketch2DOptions Sketch2DOptions::forTesting(const std::string& storageRoot) {
		Sketch2DOptions options;
		options.storage = AppStorage(storageRoot);
		options.autosave = false;
		return options;
	}

	Sketch2DApplication::Sketch2DApplication(Sketch2DOptions options) :
		WidgetApplication(options.window),
		mOptions(std::move(options)),
		mIcons(canvas(), "rsc/icons"),
		mRenderer(mWindow->vulkan()),
		mViewport(canvas(), mRenderer),
		mThumbnails(canvas()),
		mRoot(mDocument, mViewport, initialBrush(mBrush), mIcons, SketchLibrary::rootIn(mOptions.storage)) {
		mRoot.library.setAutosave(mOptions.autosave);
		mDocument.setPaintWorkers(ThreadPlacement::workerCount());
		mDocument.setEngineThreads(ThreadPlacement::workerCount());
		addHangContributor([this]() {
			std::vector<HangReportSection> sections;
			sections.push_back({ "engine.txt", mDocument.engine().describeState() });
			std::string document = "zoom " + std::to_string(mDocument.zoom()) + "\n";
			document += "shown level " + std::to_string(LayerLevels::levelForZoom(mDocument.zoom())) + "\n";
			document += "stroke preview level " + std::to_string(mDocument.previewLevel()) + "\n";
			sections.push_back({ "document.txt", document });
			return sections;
		});
		mRoot.toolBar.layersPopup.setThumbnails(&mThumbnails);
		mRoot.library.setPreviewImages(&mPreviews);
		mRoot.toolBar.layersPopup.refresh();
		setDebugToolsEnabled(false);
		mRoot.setSessionRecording(&debugTools().recording());
		mRoot.isDebugUiShown = [this]() { return inspector().isVisible(); };
		mRoot.showDebugUi = [this](bool shown) { inspector().setVisible(shown); };
		mRoot.isFullScreen = [this]() { return mWindow->isFullScreen(); };
		mRoot.setFullScreen = [this](bool fullScreen) { mWindow->setFullScreen(fullScreen); };
		mWindow->setInputDeliveryPolicies({ DeliveryPolicy::Every, DeliveryPolicy::Accumulate });
		setMenuBarVisible(false);
		mStudioRoot = std::make_unique<StudioRoot>(mRoot, mBrush, mIcons);
		ui().setInteractionMode(
			mWindow->platformInfo().isTouchFirst() ? ui::InteractionMode::Touch : ui::InteractionMode::Pointer
		);
		setRoot(*mStudioRoot);
	}

	Sketch2DApplication::~Sketch2DApplication() {
		mRoot.library.saveIfChanged();
		debugTools().recorder().cancel();
		mRoot.setSessionRecording(nullptr);
		clearRoot();
		mStudioRoot.reset();
	}

	void Sketch2DApplication::processFrame(EventHandler* eventHandler, halnf deltaTime) {
		WidgetApplication::processFrame(eventHandler, deltaTime);
		if (mUpdates) mUpdates->poll(ui());
		const bool focused = mWindow->isFocused();
		if (mWasFocused && !focused) mRoot.library.saveIfChanged();
		mWasFocused = focused;
	}

	HangSettings Sketch2DApplication::hangSettings() const {
		HangSettings settings = WidgetApplication::hangSettings();
		settings.application = "sketch2d";
		settings.build = SKETCH2D_REVISION;
		return settings;
	}

	void Sketch2DApplication::checkForUpdates(UpdateBackend& backend) {
		mRoot.versionText = "build " + std::to_string(backend.installedVersionCode()) + " (" SKETCH2D_REVISION ")";
		mUpdates = std::make_unique<UpdateManager>(backend);
		mUpdates->checkInBackground();
	}
}
