#include "rigid_transform.h"

using namespace OpenVR;

using namespace vr;
using namespace Eigen;

RigidTransform::RigidTransform(Eigen::Vector3f translation,
							   Eigen::Quaternionf rotation,
							   Eigen::Vector3f scale)
	: m_translation(translation),
	  m_rotation(rotation),
	  m_scale(scale) {
}

RigidTransform::RigidTransform(const RigidTransform& rigidTransform)
	: m_translation(rigidTransform.GetTranslation()),
	  m_rotation(rigidTransform.GetRotation()),
	  m_scale(rigidTransform.GetScale()) {
}

RigidTransform::RigidTransform(const HmdMatrix34_t& vrMatrix) {
	m_translation.x() = vrMatrix.m[0][3];
	m_translation.y() = vrMatrix.m[1][3];
	m_translation.z() = -vrMatrix.m[2][3];

	Matrix3f matrix;

	matrix(0, 0) = vrMatrix.m[0][0];
	matrix(0, 1) = vrMatrix.m[0][1];
	matrix(0, 2) = -vrMatrix.m[0][2];

	matrix(1, 0) = vrMatrix.m[1][0];
	matrix(1, 1) = vrMatrix.m[1][1];
	matrix(1, 2) = -vrMatrix.m[1][2];

	matrix(2, 0) = -vrMatrix.m[2][0];
	matrix(2, 1) = -vrMatrix.m[2][1];
	matrix(2, 2) = vrMatrix.m[2][2];

	m_scale.x() = matrix.col(0).norm();
	m_scale.y() = matrix.col(1).norm();
	m_scale.z() = matrix.col(2).norm();

	matrix.col(0).normalize();
	matrix.col(1).normalize();
	matrix.col(2).normalize();

	m_rotation = matrix;

	m_rotation.normalize();
}

RigidTransform::~RigidTransform() {
}

const Eigen::Vector3f& RigidTransform::GetTranslation() const {
	return m_translation;
}

const Eigen::Quaternionf& RigidTransform::GetRotation() const {
	return m_rotation;
}

const Eigen::Vector3f& RigidTransform::GetScale() const {
	return m_scale;
}

vr::HmdMatrix34_t RigidTransform::ToVRMatrix() const {
	Matrix3f matrix = m_rotation.normalized().toRotationMatrix();

	matrix.col(0) *= m_scale.x();
	matrix.col(1) *= m_scale.y();
	matrix.col(2) *= m_scale.z();

	vr::HmdMatrix34_t result;

	result.m[0][0] = matrix(0, 0);
	result.m[0][1] = matrix(0, 1);
	result.m[0][2] = -matrix(0, 2);
	result.m[0][3] = m_translation.x();

	result.m[1][0] = matrix(1, 0);
	result.m[1][1] = matrix(1, 1);
	result.m[1][2] = -matrix(1, 2);
	result.m[1][3] = m_translation.y();

	result.m[2][0] = -matrix(2, 0);
	result.m[2][1] = -matrix(2, 1);
	result.m[2][2] = matrix(2, 2);
	result.m[2][3] = -m_translation.z();

	return result;
}

float RigidTransform::GetTranslationX() const {
	Vector3f origin = m_rotation.inverse() * m_translation;

	return origin.x();
}

float RigidTransform::GetTranslationY() const {
	Vector3f origin = m_rotation.inverse() * m_translation;

	return origin.y();
}

float RigidTransform::GetTranslationZ() const {
	Vector3f origin = m_rotation.inverse() * m_translation;

	return origin.z();
}

void RigidTransform::TranslateX(float offset) {
	Vector3f origin = m_rotation.inverse() * m_translation;

	origin.x() += offset;

	m_translation = m_rotation * origin;
}

void RigidTransform::TranslateY(float offset) {
	Vector3f origin = m_rotation.inverse() * m_translation;

	origin.y() += offset;

	m_translation = m_rotation * origin;
}

void RigidTransform::TranslateZ(float offset) {
	Vector3f origin = m_rotation.inverse() * m_translation;

	origin.z() += offset;

	m_translation = m_rotation * origin;
}

void RigidTransform::SetTranslation(float x, float y, float z) {
	Vector3f origin = m_rotation.inverse() * m_translation;

	origin.x() = x;
	origin.y() = y;
	origin.z() = z;

	m_translation = m_rotation * origin;
}

float RigidTransform::GetRotationX() const {
	Vector3f euler = m_rotation.toRotationMatrix().eulerAngles(0, 1, 2);

	return euler.x() * RAD2DEG;
}

float RigidTransform::GetRotationY() const {
	Vector3f euler = m_rotation.toRotationMatrix().eulerAngles(0, 1, 2);

	return euler.y() * RAD2DEG;
}

float RigidTransform::GetRotationZ() const {
	Vector3f euler = m_rotation.toRotationMatrix().eulerAngles(0, 1, 2);

	return euler.z() * RAD2DEG;
}

void RigidTransform::RotateX(float degree) {
	AngleAxisf angle(degree * DEG2RAD, Vector3f::UnitX());

	m_rotation = m_rotation * angle;
}

void RigidTransform::RotateY(float degree) {
	AngleAxisf angle(degree * DEG2RAD, Vector3f::UnitY());

	m_rotation = m_rotation * angle;
}

void RigidTransform::RotateZ(float degree) {
	AngleAxisf angle(degree * DEG2RAD, Vector3f::UnitZ());

	m_rotation = m_rotation * angle;
}

void RigidTransform::SetRotation(float x, float y, float z) {
	AngleAxisf angleX(x * DEG2RAD, Vector3f::UnitX());
	AngleAxisf angleY(y * DEG2RAD, Vector3f::UnitY());
	AngleAxisf angleZ(z * DEG2RAD, Vector3f::UnitZ());

	m_rotation = angleX * angleY * angleZ;
}

float RigidTransform::GetScaleX() const {
	return m_scale.x();
}

float RigidTransform::GetScaleY() const {
	return m_scale.y();
}

float RigidTransform::GetScaleZ() const {
	return m_scale.z();
}

void RigidTransform::ScaleX(float factor) {
	m_scale.x() *= factor;
}

void RigidTransform::ScaleY(float factor) {
	m_scale.y() *= factor;
}

void RigidTransform::ScaleZ(float factor) {
	m_scale.z() *= factor;
}

void RigidTransform::SetScale(float x, float y, float z) {
	m_scale.x() = x;
	m_scale.y() = y;
	m_scale.z() = z;
}
