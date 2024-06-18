
#include "Editor.hpp"

using namespace tp;

void Editor::loadDefaults() {
	mScene.load("rsc/scene/script.lua");
}

uint4 Editor::getViewportTexID() {
	return mRasterRenderer.getRenderBufferID();
}

void Editor::renderViewport() {
	mRasterRenderer.render(mScene, mViewportSize);
}

void Editor::setViewportSize(const Vec2F& size) {
	// TODO remove
	mScene.mCamera.rotate(0.01f, 0.0);

	mRasterRenderer.getRenderBuffer()->resize(size);
	mScene.mCamera.setRatio(size.y / size.x);
}
