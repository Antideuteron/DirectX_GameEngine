#pragma once

class ObjLoader
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
  ObjLoader() {};
  ~ObjLoader() {};

};
