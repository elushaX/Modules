
#include "Editor.hpp"

#include "GraphicsApi.hpp"

using namespace tp;

Editor::Editor() {

	{ // create path tracer gpu texture
		glGenTextures(1, &mPathRenderTexture);
		glBindTexture(GL_TEXTURE_2D, mPathRenderTexture);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, 100, 100, 0, GL_RGBA, GL_FLOAT, nullptr);
		glBindTexture(GL_TEXTURE_2D, 0);
	}
}

void Editor::renderViewport() {
	switch (mRenderType) {
		case RenderType::RASTER:
			mRasterRenderer.beginRender(mScene, mScene.mRenderSettings.size);
			mRasterRenderer.renderDefault(mScene);
			if (mActive) mRasterRenderer.renderOutline(mScene.mCamera, *mActive);
			mRasterRenderer.endRender();
			break;

		case RenderType::PATH_TRACER:
			{
				// TODO : include in render viewport method
				// renderPathFrame();
				break;
			}

		default:
			break;
	}
}

uint4 Editor::getViewportTexID() {
	switch (mRenderType) {
		case RenderType::PATH_TRACER:
			return mPathRenderTexture;

		case RenderType::RASTER:
			return mRasterRenderer.getRenderBufferID();

		default:
			return 0;
	}
}

void Editor::loadDefaults() {
	mScene.load("rsc/scene/script.lua");
	mResetCamera = mScene.mCamera;
}

void Editor::setViewportSize(const Vec2F& size) {
	if (size.x <= 0 || size.y <= 0) return;

	// TODO remove
	// mScene.mCamera.rotate(0.01f, 0.0);

	mScene.mRenderSettings.size = size;

	mRasterRenderer.getRenderBuffer()->resize(size);
	mScene.mCamera.setRatio(size.y / size.x);
}

Editor::~Editor() {
	glDeleteTextures(1, &mPathRenderTexture);
}

void Editor::renderPathFrame() {
	mScene.updateCache();
	mPathRenderer.render(mScene, mPathTracerBuffers, mScene.mRenderSettings);
	sendBuffersToGPU();
}

void Editor::sendBuffersToGPU() {
	glBindTexture(GL_TEXTURE_2D, mPathRenderTexture);
	const auto size = mPathTracerBuffers.color.size();
	glTexImage2D(
		GL_TEXTURE_2D,
		0,
		GL_RGBA32F,
		(GLsizei) size.x,
		(GLsizei) size.y,
		0,
		GL_RGBA,
		GL_FLOAT,
		mPathTracerBuffers.color.getBuff()
	);
	glBindTexture(GL_TEXTURE_2D, 0);
}

void Editor::denoisePathRenderBuffers() {
	denoise();
	sendBuffersToGPU();
}

void Editor::setRenderType(Editor::RenderType type) {
	mRenderType = type;
}

void Editor::navigationOrbit(const Vec2F& delta) {
	mScene.mCamera.rotate(delta.x, delta.y);
}

void Editor::navigationPan(const Vec2F& pos, const Vec2F& prevPos) {
	mScene.mCamera.move(pos, prevPos);
}

void Editor::navigationZoom(halnf factor) {
	mScene.mCamera.zoom(factor);
}

void Editor::navigationReset() {
	mScene.mCamera = mResetCamera;
	// mScene.mCamera.lookAtPoint({ 0, 0, 0 }, { 1, 5, 1 }, { 0, 0, 1 });
}

void Editor::selectObject(const Vec2F& screenPos) {
	mScene.updateCache();

	auto dir = (mScene.mCamera.project(screenPos) - mScene.mCamera.getPos()).normalize();
	Ray ray = Ray( dir, mScene.mCamera.getPos() );

	auto rayCastData = mScene.castRay(ray, 1000);
	mActive = rayCastData.obj;
}

Object* Editor::getActiveObject() { return mActive; }

Scene* Editor::getScene() { return &mScene; }