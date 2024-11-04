#include "RasterRender.hpp"
#include "GPUBuffers.hpp"

using namespace tp;

RasterRender::RasterRender() :
	mRenderBuffer({ 100, 100 })
{
	mDefaultShader.load("rsc/shaders/default.vert", nullptr, "rsc/shaders/default.frag", true);
	mSolidShader.load("rsc/shaders/default.vert", nullptr, "rsc/shaders/solid.frag", true);
}

RasterRender::~RasterRender() {}

uint4 RasterRender::getRenderBufferID() { return mRenderBuffer.texId(); }

Vec2F RasterRender::getBufferSize() { return mRenderBuffer.getSize(); }

void RasterRender::bindCameraShaderAttributes(Mat4F cameraMat) {
	static auto camera = (GLint) mDefaultShader.getu("Camera");
	glUniformMatrix4fv(camera, 1, true, &cameraMat[0][0]);
}

void RasterRender::bindObjectShaderAttributes(const Object& object) {
	static auto origin = (GLint) mDefaultShader.getu("Origin");
	static auto basis = (GLint) mDefaultShader.getu("Basis");

	Mat4F basisMat = toMat4(object.mTopology.Basis);
	Vec4F originPoint = toVec4(object.mTopology.Origin);

	glUniform4fv(origin, 1, &originPoint[0]);
	glUniformMatrix4fv(basis, 1, false, &basisMat[0][0]);
}

void RasterRender::renderDefault(const Scene& geometry) {
	mDefaultShader.bind();

	Mat4F cameraMat = geometry.mCamera.calculateTransformationMatrix();

	glEnable(GL_DEPTH_TEST);

	// glPolygonMode(GL_FRONT, GL_LINE);
	// glPolygonMode(GL_BACK, GL_LINE);

	// auto rotator = Mat3F::rotatorZ(alnf(0.001));

	bindCameraShaderAttributes(cameraMat);

	for (auto object : geometry.mObjects) {
		// object->mTopology.Origin += 0.01;
		// object->mTopology.Basis = rotator * object->mTopology.Basis;

		bindObjectShaderAttributes(object.data());
		object->mGUPBuffers->drawCall();
	}

	mDefaultShader.unbind();
}

void RasterRender::renderOutline(const Camera& camera, const Object& object) {
	glEnable(GL_STENCIL_TEST);

	mSolidShader.bind();

	bindCameraShaderAttributes(camera.calculateTransformationMatrix());
	bindObjectShaderAttributes(object);

	object.mGUPBuffers->drawCall();

	mSolidShader.unbind();

	glDisable(GL_STENCIL_TEST);
}

void RasterRender::render(const Scene& geometry, const Vec2<ualni>& size) {

	for (auto object : geometry.mObjects) {
		if (!object->mGUPBuffers) {
			object->mGUPBuffers = new ObjectBuffers(&object.data());
		}
	}

	mRenderBuffer.mClearCol = { 0.0f, 0.0f, 0.0f, 0.f };

	mRenderBuffer.beginDraw();
	mRenderBuffer.clear();

	renderDefault(geometry);

	// if (geometry.mObjects.size()) renderOutline(geometry.mCamera, geometry.mObjects.first());

	// glPolygonMode(GL_FRONT, GL_FILL);
	// glPolygonMode(GL_BACK, GL_FILL);

	mRenderBuffer.endDraw();
}

RenderBuffer* RasterRender::getRenderBuffer() { return &mRenderBuffer; }
