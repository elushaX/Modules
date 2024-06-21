#include "RasterRender.hpp"
#include "GPUBuffers.hpp"

using namespace tp;

RasterRender::RasterRender() :
	mRenderBuffer({ 100, 100 })
{
	mDefaultShader.load("rsc/shaders/default.vert", nullptr, "rsc/shaders/default.frag", true);
}

RasterRender::~RasterRender() {}

uint4 RasterRender::getRenderBufferID() { return mRenderBuffer.texId(); }

Vec2F RasterRender::getBufferSize() { return mRenderBuffer.getSize(); }

void RasterRender::render(const Scene& geometry, const Vec2<ualni>& size) {

	for (auto object : geometry.mObjects) {
		if (!object->mGUPBuffers) {
			object->mGUPBuffers = new ObjectBuffers(&object.data());
		}
	}

	mRenderBuffer.mClearCol = { 0.0f, 0.0f, 0.0f, 0.f };

	mRenderBuffer.beginDraw();
	mRenderBuffer.clear();

	mDefaultShader.bind();

	Mat4F cameraMat = geometry.mCamera.calculateTransformationMatrix();

	glEnable(GL_DEPTH_TEST);

	// glPolygonMode(GL_FRONT, GL_LINE);
	// glPolygonMode(GL_BACK, GL_LINE);

	for (auto object : geometry.mObjects) {

		static auto origin = (GLint) mDefaultShader.getu("Origin");
		static auto basis = (GLint) mDefaultShader.getu("Basis");
		static auto camera = (GLint) mDefaultShader.getu("Camera");

		Mat4F basisMat;
		Vec4F originPoint;

		glUniform4fv(origin, 1, &originPoint[0]);
		glUniformMatrix4fv(basis, 1, false, &basisMat[0][0]);
		glUniformMatrix4fv(camera, 1, true, &cameraMat[0][0]);

		object->mGUPBuffers->drawCall();
	}

	mDefaultShader.unbind();


	// glPolygonMode(GL_FRONT, GL_FILL);
	// glPolygonMode(GL_BACK, GL_FILL);

	mRenderBuffer.endDraw();
}

RenderBuffer* RasterRender::getRenderBuffer() { return &mRenderBuffer; }
