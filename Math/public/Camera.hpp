#pragma once

#include "Mat.hpp"
#include "Ray.hpp"

namespace tp {
	class Camera {
		Vec3F mPos, mTarget, mUp;
		halnf mFOV = (halnf) (PI) / 4;
		halnf mNear = 0.001f;
		halnf mFar = 100.f;
		halnf mRatio = 1.f;

	public:
		Camera();
		~Camera() = default;

		void setRatio(halnf ratio);
		void setFOV(halnf fov);
		void setFar(halnf far);
		void setPos(Vec3F pos);

		[[nodiscard]] const Vec3F& getPos() const;
		[[nodiscard]] const Vec3F& getTarget() const;
		[[nodiscard]] Vec3F getForward() const;
		[[nodiscard]] const Vec3F& getUp() const;
		[[nodiscard]] halnf getRatio() const;
		[[nodiscard]] halnf getFOV() const;
		[[nodiscard]] halnf getFar() const;
		[[nodiscard]] halnf getNear() const;

	public:
		void lookAtPoint(const Vec3F& aTarget, const Vec3F& aPos, Vec3F aUp);

		// -1 -1 is top left 1 1 is bottom right
		void move(Vec2F aPos, Vec2F aPrevPos);

		// keeps z axis above your head
		void rotate(halnf anglex, halnf angleY);

		// -1 -1 is top left 1 1 is bottom right
		void zoom(halnf ratio);

		void offset_target(halnf val);

	public:
		[[nodiscard]] Mat4F calculateTransformationMatrix() const;
		[[nodiscard]] Mat<halnf, 4, 4> calculateProjectionMatrix() const;
		[[nodiscard]] Mat<halnf, 4, 4> calculateViewMatrix() const;

		// from -1 -1 is top left corner. z is distance to the target
		[[nodiscard]] Vec3F project(Vec2F normalized) const;
		[[nodiscard]] Vec2F project(const Vec3F& world) const;
		[[nodiscard]] static Vec2F project(const tp::Vec3F& world, const tp::Mat4F& viewMat, const tp::Mat4F& projMat);
	};
}
