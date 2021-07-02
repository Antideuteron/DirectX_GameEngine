#include "Camera.hpp"

#include "Mouse.hpp"
#include "Display.hpp"
#include "Keyboard.hpp"

constexpr XMFLOAT4 up		=	{  0.0f, -1.0f, 0.0f, 0.0f };
constexpr XMFLOAT4 left = { -1.0f,  0.0f, 0.0f, 0.0f };

XMFLOAT4 Camera::m_target		= { 0.0f,  0.0f,   1.0f, 1.0f };
XMFLOAT4 Camera::m_position	= { 0.0f,  1.0f, -10.0f, 1.0f };
XMFLOAT4 Camera::m_rotation	= { 0.0f,  0.0f,   0.0f, 0.0f };

static float aspect = 0.0f;

bool Camera::Init(const uint32_t width, const uint32_t height) noexcept
{
	aspect = static_cast<float>(width) / static_cast<float>(height);

	return true;
}

void Camera::Update()
{
	static const XMVECTOR forward = { 0.0f, 0.0f, 1.0f, 1.0f };
	static float speed = 0.025f;

	const auto& cm = Mouse::CursorMovement();

	Rotate(cm.first, cm.second);

	// TODO Fix Seitwärtsbewegung 

	const auto rrot				= XMVECTOR{ 0.0f, m_rotation.y, 0.0f, 1.0f };

	const auto direction	= XMVector3Rotate(forward, rrot);
	const auto side				= XMVector3Cross(direction, XMLoadFloat4(&up));

	if (Keyboard::IsPressed(Scancode::sc_w)) XMStoreFloat4(&m_position, XMLoadFloat4(&m_position) + speed * direction);
	if (Keyboard::IsPressed(Scancode::sc_a)) XMStoreFloat4(&m_position, XMLoadFloat4(&m_position) + speed * side);
	if (Keyboard::IsPressed(Scancode::sc_s)) XMStoreFloat4(&m_position, XMLoadFloat4(&m_position) - speed * direction);
	if (Keyboard::IsPressed(Scancode::sc_d)) XMStoreFloat4(&m_position, XMLoadFloat4(&m_position) - speed * side);

	const auto	pos = XMLoadFloat4(&m_position);

	XMStoreFloat4(&m_target, XMVectorAdd(pos, direction));
}

void Camera::Rotate(int yaw, int pitch)
{
	static float scale = 0.4f;

	const auto old = XMLoadFloat4(&m_rotation);
	const auto input = XMQuaternionRotationRollPitchYaw(XMConvertToRadians(scale * static_cast<float>(-pitch)), XMConvertToRadians(scale * static_cast<float>(yaw)), 0.0f);
	const auto _new = XMQuaternionNormalize(XMQuaternionMultiply(input, old));

	XMStoreFloat4(&m_rotation, _new);
}

XMFLOAT4X4 Camera::GetViewMatrix()
{
	static XMFLOAT4X4 matrix;

	const XMVECTOR vup = XMLoadFloat4(&up);
	const XMVECTOR vtar = XMLoadFloat4(&m_target);
	const XMVECTOR vpos = XMLoadFloat4(&m_position);

	XMStoreFloat4x4(&matrix, XMMatrixLookAtRH(vpos, vtar, vup));

	return matrix;
}

XMFLOAT4X4 Camera::GetProjectionMatrix()
{
	static XMFLOAT4X4 matrix;

	XMStoreFloat4x4(&matrix, XMMatrixPerspectiveFovRH(75.0f, aspect, 0.01f, 10000.0f));

	return matrix;
}


