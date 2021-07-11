#include "Camera.hpp"

#include "Mouse.hpp"
#include "Display.hpp"
#include "Keyboard.hpp"

static constexpr float fov = XMConvertToRadians(75.0f);
static constexpr float farDistance = 20.0f;
static constexpr float nearDistance = 0.01f;

XMFLOAT3 Camera::m_Position = { 0.0f, 1.5f, -10.0f };
XMFLOAT4 Camera::m_Rotation;
BoundingVolume Camera::m_BoundingVolume;

float	Camera::m_YAW = 0.0f;
float Camera::m_PITCH = 0.0f;

std::array<Vertex, 8> Camera::m_FrustumPoints;

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

	const auto farCenter = XMLoadFloat3(&m_Position) - XMVECTOR{ 0.0f, 0.0f, farDistance };
	const auto nearCenter = XMLoadFloat3(&m_Position) - XMVECTOR{ 0.0f, 0.0f, nearDistance };

	const float farHeight = 2.0f * tanf(fov * 0.5f) * farDistance;
	const float nearHeight = 2.0f * tanf(fov * 0.5f) * nearDistance;

	const float farWidth = aspect * farHeight;
	const float nearWidth = aspect * nearHeight;

	constexpr auto up = XMVECTOR{ 0.0f, 1.0f, 0.0f };
	constexpr auto right = XMVECTOR{ 1.0f, 0.0f, 0.0f };

	const auto farTopLeft = farCenter + up * (farHeight * 0.5f) - right * (farWidth * 0.5f);
	const auto farTopRight = farCenter + up * (farHeight * 0.5f) + right * (farWidth * 0.5f);
	const auto farBottomLeft = farCenter - up* (farHeight * 0.5f) - right * (farWidth * 0.5f);
	const auto farBottomRight = farCenter - up * (farHeight * 0.5f) + right * (farWidth * 0.5f);

	const float neg = m_Position.y * (nearHeight * 0.5f) - m_Position.x * (nearWidth * 0.5f);
	const float pos = m_Position.y * (nearHeight * 0.5f) + m_Position.x * (nearWidth * 0.5f);

	const auto nearTopLeft = nearCenter + XMVECTOR{ neg, neg, neg };
	const auto nearTopRight = nearCenter + XMVECTOR{ pos, pos, pos };
	const auto nearBottomLeft = nearCenter - XMVECTOR{ neg, neg, neg };
	const auto nearBottomRight = nearCenter - XMVECTOR{ pos, pos, pos };

	m_FrustumPoints[0] = Vertex(nearTopLeft);
	m_FrustumPoints[1] = Vertex(nearBottomLeft);
	m_FrustumPoints[2] = Vertex(nearBottomRight);
	m_FrustumPoints[3] = Vertex(nearTopRight);

	m_FrustumPoints[4] = Vertex(farTopLeft);
	m_FrustumPoints[5] = Vertex(farBottomLeft);
	m_FrustumPoints[6] = Vertex(farBottomRight);
	m_FrustumPoints[7] = Vertex(farTopRight);

	std::vector<Vertex> vertices;

	for (const auto& vertex: m_FrustumPoints) vertices.push_back(vertex);

	m_BoundingVolume = BoundingVolume(vertices);

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

	m_BoundingVolume.Update(&m_Position, &m_Rotation);
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
	static float speed = 0.05f * (Keyboard::IsPressed(sc_shiftLeft) ? 2.5f : 1.0f);

	Log::Info((std::wstringstream() << speed).str());

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


