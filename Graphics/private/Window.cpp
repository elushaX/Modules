#include "Window.hpp"
#include "WindowContext.hpp"

#include "WindowInputDispatcher.hpp"

using namespace tp;

WindowIdentity WindowIdentity::generic() { return { MODULES_GENERIC_APP_ID, MODULES_GENERIC_APP_NAME }; }

Window::Window(Vec2F size, const WindowIdentity& identity) :
	mSize(size),
	mContext(new Context()),
	mInput(std::make_unique<WindowInputDispatcher>()) {
	mContext->platform = createPlatformWindow(*this, size, identity);
	mContext->vulkan = new VulkanContext(*mContext->platform);
}

Window::~Window() {
	delete mContext->vulkan;
	mContext->platform.reset();
	delete mContext;
}

Window* Window::createWindow(Vec2F size, const WindowIdentity& identity) { return new Window(size, identity); }

void Window::destroyWindow(Window* window) { delete window; }

bool Window::shouldClose() const { return mContext->platform->shouldClose(); }

void Window::requestClose() { mContext->platform->requestClose(); }

void Window::processEvents(bool wait) { mContext->platform->dispatchEvents(wait ? -1.0f : 0.0f); }

void Window::processEventsFor(halnf timeoutMs) { mContext->platform->dispatchEvents(timeoutMs > 0 ? timeoutMs : 0.0f); }

void Window::wake() { mContext->platform->wake(); }

void Window::flushPendingInput() { mContext->platform->flushPendingInput(); }

bool Window::isWaitingForFrame() const { return mContext->platform->isWaitingForFrame(); }

bool Window::hasPendingWork() const {
	const InputCoalescer& coalescer = mContext->platform->coalescer();
	return !coalescer.isEmpty() || coalescer.hasPendingConfigure();
}

void Window::setInputDeliveryPolicies(const InputDeliveryPolicies& policies) {
	mContext->platform->coalescer().setPolicies(policies);
}

const CoalescingStatistics& Window::inputStatistics() const { return mContext->platform->coalescer().statistics(); }

uint64_t Window::swapchainRecreations() const { return mContext->vulkan->swapchainRecreations(); }

halnf Window::refreshIntervalMs() const { return mContext->platform->refreshIntervalMs(); }

void Window::waitForReady() {
	mContext->platform->waitUntilConfigured();
	mContext->vulkan->syncSwapchainToWindow();
}

void Window::draw() {}

bool Window::frameBegin() {
	const VkExtent2D extent = mContext->platform->framebufferExtent();
	return mContext->vulkan->beginFrame((int) extent.width, (int) extent.height);
}

void Window::frameEnd() {
	mContext->platform->commitFrameGeometry();
	mContext->vulkan->endFrame();
}

const Vec2F& Window::getSize() const { return mSize; }

Vec2F Window::getFramebufferSize() const {
	const VkExtent2D extent = mContext->platform->framebufferExtent();
	return { (halnf) extent.width, (halnf) extent.height };
}

halnf Window::getContentScale() const { return mContext->platform->scale(); }

const WindowFrame& Window::frame() const { return mContext->platform->frame(); }

PlatformInfo Window::platformInfo() const { return mContext->platform->platformInfo(); }

Window::Context* Window::getContext() { return mContext; }

VulkanContext* Window::vulkan() { return mContext->vulkan; }

void Window::setEventHandler(EventHandler* eventHandler) { mInput->setEventHandler(eventHandler); }

EventHandler* Window::getEventHandler() { return mInput->eventHandler(); }

void Window::addInputObserver(WindowInputObserver* observer) { mInput->addObserver(observer); }

void Window::removeInputObserver(WindowInputObserver* observer) { mInput->removeObserver(observer); }

WindowInputObserver& Window::injectInput() { return mContext->platform->coalescer().recorder(); }

void Window::setRawInputTap(WindowInputObserver* tap) { mContext->platform->coalescer().setTap(tap); }

WindowInputObserver& Window::dispatcher() { return *mInput; }

void Window::setPlatformInputEnabled(bool enabled) {
	if (enabled == mPlatformInputEnabled) return;
	mPlatformInputEnabled = enabled;
	mContext->platform->setAcceptsInput(enabled);
}

void Window::minimize() { mContext->platform->minimize(); }

void Window::toggleMaximize() { mContext->platform->toggleMaximize(); }

bool Window::isMaximized() const { return mContext->platform->isMaximized(); }

void Window::setFullScreen(bool fullScreen) { mContext->platform->setFullScreen(fullScreen); }

bool Window::isFullScreen() const { return mContext->platform->isFullScreen(); }

bool Window::isFocused() const { return mContext->platform->isFocused(); }

void Window::beginMove() { mContext->platform->beginMove(); }

void Window::beginResize(WindowEdge edge) { mContext->platform->beginResize(edge); }

void Window::showWindowMenu(Vec2F contentPosition) { mContext->platform->showWindowMenu(contentPosition); }

void Window::setCursor(CursorShape shape) { mContext->platform->setCursor(shape); }

void Window::setClipboardText(const std::string& text) { mContext->platform->setClipboardText(text); }

std::string Window::clipboardText() { return mContext->platform->clipboardText(); }

ClipboardImage Window::clipboardImage() { return mContext->platform->clipboardImage(); }

bool Window::openUrl(const std::string& url) { return mContext->platform->openUrl(url); }

void Window::setTextInputEnabled(bool enabled) { mContext->platform->setTextInputEnabled(enabled); }

void Window::setTextInputRect(const RectF& contentRect) { mContext->platform->setTextInputRect(contentRect); }

void Window::notifyResized() {
	const Vec2F size = mContext->platform->contentSize();
	const bool changed = size != mSize;
	mSize = size;

	if (changed) mInput->onResized();
}
