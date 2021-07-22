#include "Camera.hpp"

#include "Mouse.hpp"
#include "Display.hpp"
#include "Keyboard.hpp"

static constexpr float fov = XMConvertToRadians(75.0f);
static constexpr float farDistance = 15.0f;
static constexpr float nearDistance = 0.01f;

XMFLOAT3 Camera::m_Position = { 0.0f, 1.5f, -10.0f };
XMFLOAT4 Camera::m_Rotation;
BoundingVolume Camera::m_Frustum;

float Camera::speed = 0.05f;

float	Camera::m_YAW = 0.0f;
float Camera::m_PITCH = 0.0f;

static float aspect = 0.0f;

static float wrap_angle(const float angle) noexcept
{
	if (angle < -XM_PI) return angle + XM_PI + XM_PI;
	if (angle > XM_PI) return angle - XM_PI - XM_PI;
	return angle;
}

bool Camera::Init(const uint32_t width, const uint32_t height) noexcept
{
	aspect = static_cast<float>(width) / static_cast<float>(height);

	BoundingFrustum bf;
	BoundingFrustum::CreateFromMatrix(bf, XMLoadFloat4x4(&GetProjectionMatrix()));

	std::vector<XMFLOAT3> fp;
	fp.resize(8);
	bf.GetCorners(fp.data());

	std::vector<Vertex> vertices;

	for (const auto& vertex : fp) vertices.push_back(Vertex(vertex, {}, {}));

	m_Frustum = BoundingVolume(vertices);

	return true;
}

void Camera::Update()
{
	speed = 0.05f * (Keyboard::IsPressed(sc_shiftLeft) ? 1.5f : 1.0f);
	speed *= (Keyboard::IsPressed(sc_controlLeft) ? 2.0f / 3.0f : 1.0f);

	const auto& cm = Mouse::CursorMovement();

	if (cm.first || cm.second) Camera::Rotate(cm.first, cm.second);

	XMFLOAT3 translation = { 0.0f, 0.0f, 0.0f };

	if (Keyboard::IsPressed(Scancode::sc_w)) translation.z -= 1.0f;
	if (Keyboard::IsPressed(Scancode::sc_a)) translation.x -= 1.0f;
	if (Keyboard::IsPressed(Scancode::sc_s)) translation.z += 1.0f;
	if (Keyboard::IsPressed(Scancode::sc_d)) translation.x += 1.0f;

	Translate(translation);

	m_Frustum.Update(&m_Position, &m_Rotation);
}

void Camera::Rotate(int yaw, int pitch)
{
	static float speed = -0.01f;

	m_YAW = wrap_angle(m_YAW + speed * static_cast<float>(yaw));
	m_PITCH = std::min(std::max(m_PITCH + speed * static_cast<float>(pitch), 0.995f * -XM_PIDIV2), 0.995f * XM_PIDIV2);

	const auto vrot = XMQuaternionRotationRollPitchYaw(m_PITCH, m_YAW, 0.0f);
	XMStoreFloat4(&m_Rotation, vrot);
}

void Camera::Translate(XMFLOAT3& translation) noexcept
{
	XMStoreFloat3(
		&translation,
		XMVector3Transform(
			XMLoadFloat3(&translation),
			XMMatrixRotationRollPitchYaw(m_PITCH, m_YAW, 0.0f) * XMMatrixScaling(speed, speed, speed)
		)
	);

	m_Position.x += translation.x;
	m_Position.z += translation.z;
}

XMFLOAT4X4 Camera::GetViewMatrix()
{
	static XMFLOAT4X4 matrix;
	static const XMVECTOR forward = { 0.0f, 0.0f, 1.0f, 1.0f };

	const auto lookVector = XMVector3Transform(forward, XMMatrixRotationRollPitchYaw(m_PITCH, m_YAW, 0.0f));
	const auto position = XMLoadFloat3(&m_Position);
	const auto target = position + lookVector;

	XMStoreFloat4x4(&matrix, XMMatrixLookAtLH(position, target, XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f)));

	return matrix;
}

XMFLOAT4X4 Camera::GetProjectionMatrix()
{
	static XMFLOAT4X4 matrix;

	XMStoreFloat4x4(&matrix, XMMatrixPerspectiveFovRH(fov, aspect, nearDistance, farDistance));

	return matrix;
}


