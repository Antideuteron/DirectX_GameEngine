#include "Keyboard.hpp"

static std::set<uint32_t> Down;
static std::set<uint32_t> Up;

void Keyboard::KeyUp(const uint32_t key) noexcept
{
	for (const auto up : Up)
	{
		if (up == key) return;
	}

	Up.insert(key);

	for (auto down = Down.begin(); down != Down.end(); ++down)
	{
		if (*down == key) {
			Down.erase(down);
			break;
		}
	}
}

void Keyboard::KeyDown(const uint32_t key) noexcept
{
	for (const auto down : Down)
	{
		if (down == key) return;
	}

	Down.insert(key);
}

bool Keyboard::IsPressed(const uint32_t key) noexcept
{
	for (const auto down : Down)
	{
		if (down == key) return true;
	}

	return false;
}

bool Keyboard::IsReleased(const uint32_t key) noexcept
{
	for (const auto up : Up)
	{
		if (up == key) return true;
	}

	return false;
}

void Keyboard::Update(void) noexcept
{
	Up.clear();
}
