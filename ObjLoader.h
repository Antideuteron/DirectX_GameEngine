#pragma once

class ObjLoader2
{
public:
  static void Load(
    std::string filename,
    Vertex*& outVertices,
    int& vcount,
    DWORD*& outIndices,
    int& icount
  );

private:
  ObjLoader2(void) noexcept = delete;
  ~ObjLoader2(void) noexcept = delete;

};
