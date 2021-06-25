#pragma once

struct DisplayCreateInfo
{
	uint32_t Width;
	uint32_t Height;
	bool Fullscreen;
	std::wstring Title;
};

class Display
{
public:
	Display(const DisplayCreateInfo* info);
	Display(const Display&) = delete;
	Display(const Display&&) = delete;
	~Display(void) noexcept;

	void RequestStop(void) noexcept;
	void SwitchDisplayMode(void) noexcept;

	inline uint32_t Width(void) const { return (m_Fullscreen) ? m_MonitorInfo.rcMonitor.right - m_MonitorInfo.rcMonitor.left : m_Width; }
	inline uint32_t Height(void) const { return (m_Fullscreen) ? m_MonitorInfo.rcMonitor.bottom - m_MonitorInfo.rcMonitor.top : m_Height; }

	inline HWND Handle(void) const { return m_hWnd; }
	inline const bool IsFullscreen(void) const { return m_Fullscreen; }

protected:
private:
	HWND m_hWnd;
	WNDCLASS m_WndClass;
	uint32_t m_Width;	
	uint32_t m_Height;
	bool m_Fullscreen;

	std::wstring m_Title;

	MONITORINFO m_MonitorInfo;
	WINDOWPLACEMENT m_wpPrev;

	friend class Context;
};
