#include "SketchGUI.hpp"
#include "SimpleLayouts.hpp"

using namespace tp;

SketchRenderWidget::SketchRenderWidget(Canvas& canvas, Vec2F renderResolution) :
	mRenderer(renderResolution) {
	mImage = canvas.createImageFromTextId(mRenderer.getBuff()->texId(), mRenderer.getBuff()->getSize());
	mCanvas = &canvas;

	setSizePolicy(SizePolicy::Expanding, SizePolicy::Expanding);

	setDebug("render", {});
}

SketchRenderWidget::~SketchRenderWidget() { mCanvas->deleteImageHandle(mImage); }

void SketchRenderWidget::draw(Canvas& canvas) {
	auto area = getArea().relative();
	if (mProject) {
		mRenderer.renderToTexture(mProject, area.size);
		canvas.drawImage(area, &mImage, 0, 1, 12);
	}
}

void SketchRenderWidget::setProject(Project* project) { mProject = project; }

void SketchViewportWidget::setProject(Project* project) {
	mProject = project;
	mProject->mBackgroundColor = { 0.13f, 0.13f, 0.13f, 0.8f };
}

void SketchViewportWidget::setColor(const RGBA& color) {
	((PencilBrush*) mProject->mBrushes.get("pencil"))->mCol = color;
}

void SketchViewportWidget::setRenderWidget(SketchRenderWidget* widget) { addChild(widget); }

void SketchViewportWidget::process(const EventHandler& events) {
	if (!mProject) return;

	auto area = getArea().relative();
	Vec2F pointer = events.getPointer();
	auto crs = events.getPointer();

	if (!area.isInside(events.getPointer())) return;

	crs.x /= area.z;
	crs.y /= area.w;
	crs = (crs - 0.5) * 2;

	if (events.isPressed(InputID::MOUSE1)) {
		mAction = true;
		lockFocus();

	} else if (events.isReleased(InputID::MOUSE1)) {
		mAction = false;
		freeFocus();
	}

	mProject->mCamera.setRatio(area.w / area.z);

	if (mAction) {
		Vec2F relativePos = ((pointer / area.size) - 0.5f) * 2.f;
		Vec2F relativePosPrev = ((mActionPosAbsolutePrev / area.size) - 0.5f) * 2.f;
		Vec2F relativeDelta = relativePos - relativePosPrev;

		if (events.isDown(InputID::LEFT_SHIFT)) {
			mProject->mCamera.move(relativePos, relativePosPrev);
		} else if (events.isDown(InputID::LEFT_ALT)) {
			mProject->mCamera.rotate(-relativeDelta.x * halnf(PI), -relativeDelta.y * halnf(PI));
		} else if (events.isDown(InputID::LEFT_CONTROL)) {
			halnf factor = pointer.y / mActionPosAbsolutePrev.y;
			mProject->mCamera.zoom(factor);
		} else {
			mProject->sample(events.getPointerPressure(), area.w / area.z, crs);
		}

	} else {
		mProject->sample(0, area.w / area.z, crs);
	}

	mActionPosAbsolutePrev = pointer;
}

SketchViewportWidget::SketchViewportWidget() {
	setLayout(new ToolBarLayout(this));

	setDebug("viewport", {});
}

void SketchViewportWidget::setToolbarWidget(Widget* widget) {
	addChild(widget);
	widget->bringToFront();
}

ToolbarWidget::ToolbarWidget() {
	setDirection(false);
	setDebug("toolbar", {});

	addToToolbar(&mFileHover);
	addToToolbar(&mPencilHover);
	addToToolbar(&mViewHover);

	mFileHover.setText("File");
	mViewHover.setText("View");
	mPencilHover.setText("Brush");
}

void ToolbarWidget::addToToolbar(Widget* button) {
	button->getLayout()->setMinSize({60, 30});
	getContainer()->addChild(button);
}

SketchGUI::SketchGUI() :
	DockWidget() {

	setCenterWidget(&mViewport);
	dockWidget(&mPanel, DockLayout::RIGHT);
	toggleWidgetVisibility(DockLayout::RIGHT);

	mPanel.addToMenu(&mControls);
	mPanel.setText("Controls");
	mPanel.setArea({ 110, 110, 300, 500 });

	mControls.setText("Tools");
	mControls.addToMenu(&mSelectPencil);
	mControls.addToMenu(&mSelectEraser);
	mControls.setArea({ 0, 0, 300, 400 });

	// brush
	{
		auto popup = mToolbar.mPencilHover.getPopup();
		popup->addChild(&mColorPicker);
		popup->addChild(&mBrushSizeSlider);

		mColorPicker.setArea({10, 10, 300, 300});
	}

	// view
	{
		auto popup = mToolbar.mViewHover.getPopup();
		popup->addChild(&mToggleSidePanel);

		mToggleSidePanel.setText("Toggle Panel");
		mToggleSidePanel.setAction([this](){
			toggleWidgetVisibility(getSide(&mPanel));
		});
	}

	mSelectPencil.setAction([this](){ mProject->setPencil(); });
	mSelectPencil.setText("Pencil");

	mSelectEraser.setAction([this](){ mProject->setEraser(); });
	mSelectEraser.setText("Eraser TODO");
}

void SketchGUI::createRenderWidget(Canvas* canvas, const Vec2F& renderResolution) {
	mRenderWidget = new SketchRenderWidget(*canvas, renderResolution);

	mViewport.setRenderWidget(mRenderWidget);
	mViewport.setToolbarWidget(&mToolbar);
}

void SketchGUI::setProject(Project* project) {
	mProject = project;
	mViewport.setProject(project);
	if (mRenderWidget) mRenderWidget->setProject(project);
}

SketchGUI::~SketchGUI() { delete mRenderWidget; }

void SketchGUI::process(const EventHandler& events) {
	DockWidget::process(events);

	if (!mProject) return;

	auto pencil = dynamic_cast<PencilBrush*>(mProject->mBrushes.get("pencil"));

	pencil->mCol = mColorPicker.mColorWheel.color;
	pencil->mSize = mBrushSizeSlider.val() * 0.2f + 0.001f;
}

void SketchGUI::draw(Canvas& canvas) { canvas.rect(getArea().relative(), mBackgroundColor, mRounding); }
