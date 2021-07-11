#include "Camera.hpp"

#include "Mouse.hpp"
#include "Display.hpp"
#include "Keyboard.hpp"

XMFLOAT3 Camera::m_position	= { 0.0f, 1.5f, -10.0f};

float	Camera::m_YAW = 0.0f;
float Camera::m_PITCH = 0.0f;

std::array<XMFLOAT3, 8> Camera::m_FrustumPoints;

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



	return true;
}

void Camera::Update()
{
	const auto& cm = Mouse::CursorMovement();

	if (cm.first || cm.second) Camera::Rotate(cm.first, cm.second);

	XMFLOAT3 translation = { 0.0f, 0.0f, 0.0f };

	if (Keyboard::IsPressed(Scancode::sc_w)) translation.z -= 1.0f;
	if (Keyboard::IsPressed(Scancode::sc_a)) translation.x -= 1.0f;
	if (Keyboard::IsPressed(Scancode::sc_s)) translation.z += 1.0f;
	if (Keyboard::IsPressed(Scancode::sc_d)) translation.x += 1.0f;

	Translate(translation);
}

void Camera::Rotate(int yaw, int pitch)
{
	static float speed = -0.01f;

	m_YAW = wrap_angle(m_YAW + speed * static_cast<float>(yaw));
	m_PITCH = std::min(std::max(m_PITCH + speed * static_cast<float>(pitch), 0.995f * -XM_PIDIV2), 0.995f * XM_PIDIV2);
}

void Camera::Translate(XMFLOAT3& translation) noexcept
{
	static float speed = 0.05f;

	XMStoreFloat3(
		&translation,
		XMVector3Transform(
			XMLoadFloat3(&translation),
			XMMatrixRotationRollPitchYaw(m_PITCH, m_YAW, 0.0f) * XMMatrixScaling(speed, speed, speed)
		)
	);

	m_position.x += translation.x;
	m_position.z += translation.z;
}

XMFLOAT4X4 Camera::GetViewMatrix()
{
	static XMFLOAT4X4 matrix;
	static const XMVECTOR forward = { 0.0f, 0.0f, 1.0f, 1.0f };

	const auto lookVector = XMVector3Transform(forward, XMMatrixRotationRollPitchYaw(m_PITCH, m_YAW, 0.0f));
	const auto position = XMLoadFloat3(&m_position);
	const auto target = position + lookVector;
	
	XMStoreFloat4x4(&matrix, XMMatrixLookAtLH(position, target, XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f)));

	return matrix;
}

XMFLOAT4X4 Camera::GetProjectionMatrix()
{
	static XMFLOAT4X4 matrix;

	XMStoreFloat4x4(&matrix, XMMatrixPerspectiveFovRH(XMConvertToRadians(75.0f), aspect, 0.01f, 10000.0f));

	return matrix;
}


