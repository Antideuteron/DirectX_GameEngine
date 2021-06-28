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
  m_mesh->LoadResources(device, commandList);

  std::vector<Vertex> vertices;

  vertices.resize(m_mesh->VertexCount());
  memcpy(vertices.data(), m_mesh->Vertices(), sizeof(Vertex) * m_mesh->VertexCount());

  m_BoundingVolume = BoundingVolume(vertices);
  m_BoundingVolume.Update(&m_position, &m_rotation);
}

void Model::PopulateCommandList(ComPtr<ID3D12GraphicsCommandList>& commandList, UINT8* cbAddress, D3D12_GPU_VIRTUAL_ADDRESS cbvAddress)
{
  XMVECTOR scale = XMVECTOR{ 1.0f, 1.0f, 1.0f, 1.0f };
  XMVECTOR origin = XMVECTOR{ 0.0f, 0.0f, 0.0f, 1.0f };

  XMVECTOR vrot = XMLoadFloat4(&m_rotation);
  XMVECTOR vpos = XMLoadFloat3(&m_position);

  XMStoreFloat4x4(&m_constantBuffer.wvpMat, XMMatrixAffineTransformation(scale, origin, vrot, vpos));

  //memcpy(cbAddress, &m_constantBuffer, sizeof(ConstantBuffer));

  m_mesh->PopulateCommandList(commandList, cbvAddress);
}

void Model::Release()
{
  if (m_mesh) m_mesh->Release();
}
