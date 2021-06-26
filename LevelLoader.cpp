#include "LevelLoader.h"

std::vector<std::vector<int>> LevelLoader::Load(const std::string& filename)
{
  std::vector<std::vector<int>> cells;

  std::string to;
  std::ifstream t(filename);

  while (std::getline(t, to))
  {
    std::vector<int> row;

    for (const char c : to)
    {
      if (c < '0' || c > '9') continue;

      row.emplace_back(c - 0x30);
    }

    cells.push_back(row);
  }

  return cells;
}
