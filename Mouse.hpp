#pragma once

class Mouse
{
public:
	Mouse(void) noexcept = delete;
	~Mouse(void) noexcept = delete;

	static bool Init(const HWND handle) noexcept;

	static inline void Reset(void) noexcept { s_LastCursorMovement = { 0, 0 }; }
	static void Update(const LPARAM param) noexcept;

	static const std::pair<int32_t, int32_t>& CursorMovement(void) noexcept { return s_LastCursorMovement; }

protected:
private:
	static std::pair<int32_t, int32_t> s_LastCursorMovement;

};
