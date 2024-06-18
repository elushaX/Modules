#pragma once

#include "Scene.hpp"
#include "GraphicsApi.hpp"

namespace tp {

	class ObjectBuffers : public GPUBuffers {
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

		void drawCall() override {
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

}