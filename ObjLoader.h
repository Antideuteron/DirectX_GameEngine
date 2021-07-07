#pragma once

class OBJLoader
{
public:
  static void Load(const std::string& filename, Vertex*& outVertices, int& vcount, DWORD*& outIndices, int& icount) noexcept;

  OBJLoader(void) noexcept = delete;
  ~OBJLoader(void) noexcept = delete;
};
