#include "Mouse.hpp"

std::pair<int32_t, int32_t> Mouse::s_LastCursorMovement = { 0, 0 };

bool Mouse::Init(const HWND handle) noexcept
{
	const RAWINPUTDEVICE raw_idev = { 0x1, 0x2, 0, handle };

	if (!RegisterRawInputDevices(&raw_idev, 1, sizeof(RAWINPUTDEVICE)))
	{
		Log::Info("Unable to register raw mouse input device");

		return false;
	}

	Log::Info("Registered raw mouse input device");

	return true;
}

void Mouse::Update(const LPARAM param) noexcept
{
	static BYTE lpb[40];

	UINT dwSize = 40;

	GetRawInputData(
		(HRAWINPUT)param,
		RID_INPUT,
		lpb,
		&dwSize,
		sizeof(RAWINPUTHEADER)
	);

	const RAWINPUT* raw = (RAWINPUT*)lpb;

	if (raw->header.dwType == RIM_TYPEMOUSE)
	{
		s_LastCursorMovement = { raw->data.mouse.lLastX, raw->data.mouse.lLastY };
	}
}
