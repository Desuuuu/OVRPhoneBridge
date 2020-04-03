#pragma once

#include <cmath>

#include <openvr.h>
#include <Eigen>

namespace OpenVR {
	class RigidTransform {
		public:
			RigidTransform(Eigen::Vector3f translation,
						   Eigen::Quaternionf rotation,
						   Eigen::Vector3f scale);
			RigidTransform(const RigidTransform& rigidTransform);
			RigidTransform(const vr::HmdMatrix34_t& vrMatrix);
			~RigidTransform();

			vr::HmdMatrix34_t ToVRMatrix() const;

			float GetTranslationX() const;
			float GetTranslationY() const;
			float GetTranslationZ() const;
			void TranslateX(float offset);
			void TranslateY(float offset);
			void TranslateZ(float offset);
			void SetTranslation(float x, float y, float z);

			float GetRotationX() const;
			float GetRotationY() const;
			float GetRotationZ() const;
			void RotateX(float degree);
			void RotateY(float degree);
			void RotateZ(float degree);
			void SetRotation(float x, float y, float z);

			float GetScaleX() const;
			float GetScaleY() const;
			float GetScaleZ() const;
			void ScaleX(float factor);
			void ScaleY(float factor);
			void ScaleZ(float factor);
			void SetScale(float x, float y, float z);

		protected:
			const Eigen::Vector3f& GetTranslation() const;
			const Eigen::Quaternionf& GetRotation() const;
			const Eigen::Vector3f& GetScale() const;

		private:
			Eigen::Vector3f m_translation;
			Eigen::Quaternionf m_rotation;
			Eigen::Vector3f m_scale;

			static constexpr float RAD2DEG = static_cast<float>(180 / M_PI);
			static constexpr float DEG2RAD = static_cast<float>(M_PI / 180);
	};
};
