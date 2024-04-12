#include "Render.hpp"

#include "GraphicsApi.hpp"

using namespace tp;

struct Shape {
	float buffer[6] = {
		0, 0, 1, 0, 0, 1,
	};

	GLuint vbo_id = 0;
	GLuint vbo_len = 6;

	Shape() {
		glGenBuffers(1, &vbo_id);
		glBindBuffer(GL_ARRAY_BUFFER, vbo_id);
		glBufferData(GL_ARRAY_BUFFER, sizeof(buffer), buffer, GL_STATIC_COPY);
	}

	void drawCall() {
		glBindBuffer(GL_ARRAY_BUFFER, vbo_id);

		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);
		glEnableVertexAttribArray(0);

		glDrawArrays(GL_TRIANGLES, 0, 3);

		glDisableVertexAttribArray(0);
		glBindVertexArray(0);
	}

	~Shape() { glDeleteBuffers(1, &vbo_id); }
};

Render::Render(Vec2F renderResolution) :
	mRenderBuffer(renderResolution) {

	mDefaultShader.load("rsc/shaders/default.vert", nullptr, "rsc/shaders/default.frag", true);
}

Render::~Render() {}

uint4 Render::getRenderBuffer() { return mRenderBuffer.texId(); }

Vec2F Render::getBufferSize() { return mRenderBuffer.getSize(); }

void Render::render(const Geometry& geometry, Vec2F size) {

	static Shape shape;

	mRenderBuffer.mClearCol = { 0.1f, 0.1f, 0.1f, 1.f };

	mRenderBuffer.beginDraw();
	mRenderBuffer.clear();

	{
		mDefaultShader.bind();

		static auto uniform_color = mDefaultShader.getu("Color");
		static auto uniform_scale = mDefaultShader.getu("Viewport");

		RGBA col = { 0, 0, 1.f, 1.f };
		RectF viewport = { 0.0f, 0.0f, 1.f, 1.f };
		viewport.w = viewport.z * (size.y / size.x);

		glUniform4fv(uniform_color, 1, &col.r);
		glUniform4fv(uniform_scale, 1, &viewport.x);

		shape.drawCall();
		
		mDefaultShader.unbind();
	}

	mRenderBuffer.endDraw();
}
