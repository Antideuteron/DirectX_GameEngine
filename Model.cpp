#include "Model.h"

#include "ObjLoader.h"

Model::Model(const std::string& object, const std::string& texture, XMFLOAT3 position, XMFLOAT4 rotation) :
  m_position(position),
  m_rotation(rotation),
  m_mesh(nullptr)
{
  Mesh::GetMesh(object, texture, m_mesh);
}

void Model::LoadResources(ComPtr<ID3D12Device>& device, ComPtr<ID3D12GraphicsCommandList>& commandList)
{
  int indexCount;
  int vertexCount;
  DWORD* indexList;
  Vertex* vertexList;

  ObjLoader::Load(m_mesh->ObjectFileName(), vertexList, vertexCount, indexList, indexCount);

  if (vertexCount && indexCount)
  {
    m_mesh->CreateVertexBuffer(device, commandList, vertexList, vertexCount);
    m_mesh->CreateIndexBuffer(device, commandList, indexList, indexCount);
    m_mesh->LoadTexture(device, commandList);
  }

  delete[] indexList;
  delete[] vertexList;
}
