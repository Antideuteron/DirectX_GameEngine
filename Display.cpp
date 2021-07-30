#include "Display.h"

#include <exception>

#include "Application.h"

Display::Display(const DisplayCreateInfo* info) :
	m_hWnd(nullptr),
	m_Width(info->Width),
	m_Height(info->Height),
	m_Title(info->Title),
	m_Fullscreen(info->Fullscreen)
{
  const wchar_t CLASS_NAME[] = L"CS5328 - Engine";

  ZeroMemory(&m_WndClass, sizeof(WNDCLASS));

  m_WndClass.lpfnWndProc = Graphics::WndProc;
  m_WndClass.hInstance = Application::WinAppInstance();
  m_WndClass.lpszClassName = CLASS_NAME;

  RegisterClass(&m_WndClass);

  m_hWnd = CreateWindowExW(
    0,
    CLASS_NAME,
    m_Title.c_str(),
    WS_OVERLAPPEDWINDOW,

    (GetSystemMetrics(SM_CXSCREEN) - m_Width) / 2, (GetSystemMetrics(SM_CYSCREEN) - m_Height) / 2,
    static_cast<int32_t>(m_Width), static_cast<int32_t>(m_Height),

    nullptr,
    nullptr,
    Application::WinAppInstance(),
    nullptr
  );

  if (m_hWnd == nullptr) throw std::runtime_error("unable to create display");

  SetWindowLong(m_hWnd, GWL_STYLE, 0);
  ShowWindow(m_hWnd, SW_SHOW);
  while (ShowCursor(0) >= 0);

  RECT rect;
  GetClientRect(m_hWnd, &rect);
  MapWindowPoints(m_hWnd, nullptr, reinterpret_cast<POINT*>(&rect), 2);
  ClipCursor(&rect);
}

Display::~Display(void) noexcept
{
  DestroyWindow(m_hWnd);
  while (ShowCursor(1) < 0);
  ClipCursor(nullptr);

  m_hWnd = nullptr;
}

void Display::RequestStop(void) noexcept
{
  PostMessage(m_hWnd, WM_DESTROY, 0, 0);
}

void Display::SwitchDisplayMode(void) noexcept
{
  m_Fullscreen = !m_Fullscreen;

  Log::Info("Switching display mode");

  if (m_Fullscreen) {
    MONITORINFO mi = { sizeof(mi) };

    if (GetWindowPlacement(m_hWnd, &m_wpPrev) && GetMonitorInfo(MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTOPRIMARY), &mi)) {
      SetWindowPos(
        m_hWnd, HWND_TOP,
        mi.rcMonitor.left, mi.rcMonitor.top,
        mi.rcMonitor.right - mi.rcMonitor.left,
        mi.rcMonitor.bottom - mi.rcMonitor.top,
        SWP_NOOWNERZORDER | SWP_FRAMECHANGED
      );

      memcpy(&m_MonitorInfo, &mi, sizeof(MONITORINFO));
    }
  }
  else
  {
    SetWindowPlacement(m_hWnd, &m_wpPrev);
    SetWindowPos(
      m_hWnd, nullptr,
      0, 0, 0, 0,
      SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED
    );
  }

  std::string buf = "New Window Size: (" + std::to_string(Width()) + "|" + std::to_string(Height()) + ")";

  Log::Info(buf);
}
