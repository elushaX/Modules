
#include "FrameBuffer.hpp"
#include "GraphicsApi.hpp"

#include <stdio.h>

void glerr(GLenum type) { printf("GL ERROR\n"); }
#define AssertGL(x)                         \
	{                                         \
		x;                                      \
		GLenum __gle = glGetError();            \
		if (__gle != GL_NO_ERROR) glerr(__gle); \
	}

using namespace tp;

RenderBuffer::RenderBuffer(const Vec2F& size) :
	mSize(size) {

	mDrawBuffers[0] = { GL_COLOR_ATTACHMENT0 };

	// --------- texture ---------
	AssertGL(glGenTextures(1, &mTextureId));
	AssertGL(glBindTexture(GL_TEXTURE_2D, mTextureId));
	// Give an empty image to OpenGL ( the last "0" )
	AssertGL(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (GLsizei) size.x, (GLsizei) size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0)
	);
	// Poor filtering. Needed
	AssertGL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
	AssertGL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));

	// --------- depth ---------
	AssertGL(glGenRenderbuffers(1, &mDepthBufferID));
	AssertGL(glBindRenderbuffer(GL_RENDERBUFFER, mDepthBufferID));
	AssertGL(glRenderbufferStorage(GL_RENDERBUFFER, GLW_CONTEXT_DEPTH_COMPONENT, (GLsizei) size.x, (GLsizei) size.y));

	// ------------ framebuffer ------------
	AssertGL(glGenFramebuffers(1, &mFrameBufferID));
	AssertGL(glBindFramebuffer(GL_FRAMEBUFFER, mFrameBufferID));
	AssertGL(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, mDepthBufferID));
	// Set "renderedTexture" as our colour attachement #0
	AssertGL(glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, mTextureId, 0));
	// Set the list of draw buffers.
	AssertGL(glDrawBuffers(1, mDrawBuffers)); // "1" is the size of DrawBuffers
	ASSERT(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

RenderBuffer::RenderBuffer(const Vec2F& size, tp::uint1 samples) :
	mSize(size) {

	mDrawBuffers[0] = { GL_COLOR_ATTACHMENT0 };

	// ------- texture ---------
	AssertGL(glGenTextures(1, &mTextureId));
	AssertGL(glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, mTextureId));
#ifdef ENV_OS_ANDROID
	AssertGL(
		glTexStorage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, GL_RGBA, (GLsizei) size.x, (GLsizei) size.y, GL_TRUE)
	);
#else
	AssertGL(
		glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, GL_RGBA, (GLsizei) size.x, (GLsizei) size.y, GL_TRUE)
	);
#endif
	// !?
	// AssertGL(glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
	// AssertGL(glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_MIN_FILTER, GL_LINEAR));

	// ------- depth -------
	AssertGL(glGenRenderbuffers(1, &mDepthBufferID));
	AssertGL(glBindRenderbuffer(GL_RENDERBUFFER, mDepthBufferID));
	AssertGL(glRenderbufferStorageMultisample(
		GL_RENDERBUFFER, samples, GLW_CONTEXT_DEPTH_COMPONENT, (GLsizei) size.x, (GLsizei) size.y
	));

	// ------- fbuff -------
	AssertGL(glGenFramebuffers(1, &mFrameBufferID));
	AssertGL(glBindFramebuffer(GL_FRAMEBUFFER, mFrameBufferID));
	AssertGL(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, mTextureId, 0));
	AssertGL(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, mDepthBufferID));
	AssertGL(glDrawBuffers(1, mDrawBuffers));
	ASSERT(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

uint4 RenderBuffer::buffId() const { return mFrameBufferID; }

void RenderBuffer::beginDraw() {
	AssertGL(glBindFramebuffer(GL_FRAMEBUFFER, mFrameBufferID));
	setViewport({ 0.f, 0.f, mSize.x, mSize.y });
}

void RenderBuffer::setViewport(const RectF& viewport) {
	AssertGL(glViewport((GLsizei) viewport.x, (GLsizei) viewport.y, (GLsizei) viewport.z, (GLsizei) viewport.w));
}

void RenderBuffer::clear() {
	AssertGL(glClearColor(mClearCol.r, mClearCol.g, mClearCol.b, mClearCol.a));
	AssertGL(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
}

void RenderBuffer::endDraw() {
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glClearColor(0, 0, 0, 0);
	glUseProgram(0);
}

RenderBuffer::~RenderBuffer() {
	glDeleteFramebuffers(1, &mFrameBufferID);
	glDeleteTextures(1, &mTextureId);
	glDeleteRenderbuffers(1, &mDepthBufferID);
}

uint4 RenderBuffer::texId() const { return mTextureId; }

const Vec2F& RenderBuffer::getSize() const { return mSize; }
