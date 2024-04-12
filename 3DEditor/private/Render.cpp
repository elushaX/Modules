#include "Render.hpp"

#include "GraphicsApi.hpp"

using namespace tp;

class ObjectBuffers {
public:
	ObjectBuffers(Object* object) {
		mObject = (object);

		auto& buff = mObject->mTopology.Points;
		auto& indices = mObject->mTopology.Indexes;

		glGenVertexArrays(1, &VAO);
		glGenBuffers(1, &VBO);
		glGenBuffers(1, &EBO);

		glBindVertexArray(VAO);

		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(Vec3F) * buff.size(), buff.getBuff(), GL_STATIC_DRAW);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(Vec3<uhalni>) * indices.size(), indices.getBuff(), GL_STATIC_DRAW);

		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
		glEnableVertexAttribArray(0);

		// glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
	}

	Object* mObject = nullptr;

	GLuint VAO = 0;
	GLuint VBO = 0;
	GLuint EBO = 0;

	void drawCall() {
		auto& indices = mObject->mTopology.Indexes;

		glBindVertexArray(VAO);

		// glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
		glDrawElements(GL_TRIANGLES, indices.size() * 3, GL_UNSIGNED_INT, nullptr);

		// glDrawArrays(GL_TRIANGLES, 0, sizeof(buffer) / (2 * sizeof(float)));

		// glBindVertexArray(0);
	}

	~ObjectBuffers() {
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

void Render::render(const Scene& geometry, Vec2F size) {

	for (auto object : geometry.mObjects) {
		if (!object->mBuffers) {
			object->mBuffers = std::make_shared<ObjectBuffers>(&object.data());
		}
	}

	mRenderBuffer.mClearCol = { 0.0f, 0.0f, 0.0f, 0.f };

	mRenderBuffer.beginDraw();
	mRenderBuffer.clear();

	mDefaultShader.bind();

	Mat4F cameraMat = geometry.mCamera.calculateTransformationMatrix();

	glEnable(GL_DEPTH_TEST);

	for (auto object : geometry.mObjects) {

		static auto origin = (GLint) mDefaultShader.getu("Origin");
		static auto basis = (GLint) mDefaultShader.getu("Basis");
		static auto camera = (GLint) mDefaultShader.getu("Camera");

		Mat4F basisMat;
		Vec4F originPoint;

		glUniform4fv(origin, 1, &originPoint[0]);
		glUniformMatrix4fv(basis, 1, false, &basisMat[0][0]);
		glUniformMatrix4fv(camera, 1, true, &cameraMat[0][0]);

		object->mBuffers->drawCall();
	}

	mDefaultShader.unbind();

	mRenderBuffer.endDraw();
}
