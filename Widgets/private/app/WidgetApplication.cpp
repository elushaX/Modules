#include "WidgetApplication.hpp"
#include "Profiler.hpp"

#include "CanvasImageLoader.hpp"
#include "CanvasPainter.hpp"
#include "Widget.hpp"

#include "VulkanContext.hpp"

#include <chrono>
#include <ctime>

using namespace tp;
using namespace tp::ui;

namespace {
	halnf millisecondsSince(time_ns start) { return (halnf) ((double) (get_time_ns() - start) / 1.0e6); }
}

WidgetApplication::WidgetApplication(const WindowIdentity& identity) :
	Application(identity),
	mTextMeasurer(std::make_unique<CanvasPainter>(*mGraphics->getCanvas())),
	mImageLoader(std::make_unique<CanvasImageLoader>(*mGraphics->getCanvas())) {
	mUi.setTextMetrics(mTextMeasurer.get());
	mUi.setImageLoader(mImageLoader.get());
	mUi.setWindowHost(this);
	mWindow->addInputObserver(&mInputTranslator);
	mDebugTools.attachTo(mFrame.menuBar);
}

WidgetApplication::~WidgetApplication() {
	mDebugTools.detach();
	mDebugTools.recorder().cancel();
	mWindow->removeInputObserver(&mInputTranslator);
	clearRoot();
	mUi.setImageLoader(nullptr);
	mUi.setWindowHost(nullptr);
}

void WidgetApplication::setRoot(Widget& root) {
	mFrame.setContent(&root);
	mUi.setRoot(&mFrame);
	mUi.setViewport(mInspector.splitWindow(mWindow->getSize()).content);
}

void WidgetApplication::clearRoot() {
	mUi.setRoot(nullptr);
	mFrame.setContent(nullptr);
}

void WidgetApplication::processFrame(EventHandler* eventHandler, halnf deltaTime) {
	mInspector.handleShortcuts(*eventHandler);
	mUi.resizeViewport(mInspector.splitWindow(mWindow->getSize()).content);
	if (mInspector.isProcessingFrozen()) return;

	const time_ns start = get_time_ns();

	const InputRouting routing = mInspector.inputRouting(eventHandler->getPointer(), mWindow->getSize());
	{
		TP_PROFILE_SCOPE("ui.dispatch");
		for (const Event& event : mInputTranslator.translate(*eventHandler, routing))
			mUi.dispatch(event);
	}
	{
		TP_PROFILE_SCOPE("ui.advance");
		mUi.advance(deltaTime);
	}

	mLastProcessMs = millisecondsSince(start);
}

void WidgetApplication::drawFrame(Canvas* canvas) {
	const time_ns start = get_time_ns();

	CanvasPainter painter(*canvas);
	{
		TP_PROFILE_SCOPE("ui.paint");
		mUi.paint(painter);
	}
	mInspector.paintOverlay(painter);

	drawOverlayInterface();
	mInspector.draw();

	mInspector.recordFrame(mLastProcessMs, millisecondsSince(start), mFramesProcessedPerSecond, mFramesDrawnPerSecond);

	const CoalescingStatistics& input = mWindow->inputStatistics();
	mInspector.recordWindowCounters(
		{ input.received,
			input.delivered,
			input.configuresReceived,
			input.configuresApplied,
			mWindow->swapchainRecreations() }
	);
}

bool WidgetApplication::forceNewFrame() { return mUi.needsFrame() || mInspector.forcesRedraw(); }

PlatformInfo WidgetApplication::platformInfo() const { return mWindow->platformInfo(); }

void WidgetApplication::minimizeWindow() { mWindow->minimize(); }

void WidgetApplication::toggleMaximizeWindow() { mWindow->toggleMaximize(); }

void WidgetApplication::closeWindow() { mWindow->requestClose(); }

bool WidgetApplication::isWindowMaximized() const { return mWindow->isMaximized(); }

void WidgetApplication::beginWindowMove() { mWindow->beginMove(); }

void WidgetApplication::showWindowMenu(Vec2F windowPoint) { mWindow->showWindowMenu(windowPoint); }

void WidgetApplication::setCursor(CursorShape shape) { mWindow->setCursor(shape); }

void WidgetApplication::setClipboardText(const std::string& text) { mWindow->setClipboardText(text); }

std::string WidgetApplication::clipboardText() { return mWindow->clipboardText(); }

ClipboardImage WidgetApplication::clipboardImage() { return mWindow->clipboardImage(); }

bool WidgetApplication::openUrl(const std::string& url) { return mWindow->openUrl(url); }

void WidgetApplication::setTextInputArea(const RectF* windowRect) {
	mWindow->setTextInputEnabled(windowRect != nullptr);
	if (windowRect) mWindow->setTextInputRect(*windowRect);
}

void WidgetApplication::setDebugToolsEnabled(bool enabled) {
	if (enabled) mDebugTools.attachTo(mFrame.menuBar);
	else mDebugTools.detach();
}

void WidgetApplication::setRawInputTap(WindowInputObserver* tap) { mWindow->setRawInputTap(tap); }

double WidgetApplication::nowMs() const {
	const auto now = std::chrono::steady_clock::now().time_since_epoch();
	return std::chrono::duration<double, std::milli>(now).count();
}

HangSettings WidgetApplication::hangSettings() const {
	HangSettings settings = Application::hangSettings();
	settings.directory = persistentAppStorage().hangsDirectory();
	return settings;
}

SessionEnvironment WidgetApplication::environment() const {
	SessionEnvironment environment;
	const HangSettings settings = hangSettings();
	environment.application = settings.application;
	environment.revision = settings.build;
	VkPhysicalDeviceProperties properties{};
	vkGetPhysicalDeviceProperties(mWindow->vulkan()->physicalDevice(), &properties);
	environment.device = properties.deviceName;
	const std::time_t now = std::time(nullptr);
	char date[64];
	std::strftime(date, sizeof(date), "%Y-%m-%d %H:%M:%S", std::localtime(&now));
	environment.date = date;
	environment.windowSize = mWindow->getSize();
	environment.contentScale = mWindow->getContentScale();
	return environment;
}
