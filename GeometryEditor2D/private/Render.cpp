#include "Render.hpp"

#include "GraphicsApi.hpp"

using namespace tp;

struct Shape {
	float buffer[8] = {
		1, 1, 1, 0, 0, 0, //
		0, 1,             // 1, 1, 0, 0,
	};

	unsigned int indices[6] = { 0, 1, 3, 1, 2, 3 };

	GLuint VAO = 0;
	GLuint VBO = 0;
	GLuint EBO = 0;

	Shape() {
		glGenVertexArrays(1, &VAO);
		glGenBuffers(1, &VBO);
		glGenBuffers(1, &EBO);

		glBindVertexArray(VAO);

		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(buffer), buffer, GL_STATIC_DRAW);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);
		glEnableVertexAttribArray(0);

		// glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
	}

	void drawCall() {
		const auto numPoints = sizeof(buffer) / (2 * sizeof(float));

		glBindVertexArray(VAO);

		// glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
		auto num = sizeof(indices) / sizeof(indices[0]);
		glDrawElements(GL_TRIANGLES, num, GL_UNSIGNED_INT, nullptr);

		// glDrawArrays(GL_TRIANGLES, 0, sizeof(buffer) / (2 * sizeof(float)));

		// glBindVertexArray(0);
	}

	~Shape() {
		glDeleteBuffers(1, &VBO);
		glDeleteVertexArrays(1, &VAO);
	}
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
		RectF viewport = { -1.0f, -1.0f, 3.f, 3.f };
		viewport.w = viewport.z * (size.y / size.x);

		glUniform4fv(uniform_color, 1, &col.r);
		glUniform4fv(uniform_scale, 1, &viewport.x);

		shape.drawCall();

		mDefaultShader.unbind();
	}

	mRenderBuffer.endDraw();
}
