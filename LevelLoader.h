#pragma once
class LevelLoader
{
public:
	LevelLoader() = delete;
	~LevelLoader() = delete;

	static std::vector<std::vector<int>> Load(const std::string& filename);
};

