#include "Model.h"

#include "Camera.hpp"

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

  m_BoundingVolume = new BoundingVolume(vertices);
  m_BoundingVolume->Update(&m_position, &m_rotation);
}

void Model::Update(int frameIndex)
{
  m_mesh->Update(frameIndex);
  m_BoundingVolume->Update(&m_position, &m_rotation);

  static const auto scale = XMVECTOR{ 1.0f, 1.0f, 1.0f, 1.0f };  // this static local variables reduce
  static const auto origin = XMVECTOR{ 0.0f, 0.0f, 0.0f, 1.0f }; // each stackframe by 16 bytes
  const auto vrot = XMLoadFloat4(&m_rotation);
  const auto vpos = XMLoadFloat3(&m_position);

  const auto view = Camera::GetViewMatrix();
  const auto proj = Camera::GetProjectionMatrix();

  const XMMATRIX m = XMMatrixAffineTransformation(scale, origin, vrot, vpos);
  const XMMATRIX v = XMLoadFloat4x4(&Camera::GetViewMatrix());
  const XMMATRIX p = XMLoadFloat4x4(&Camera::GetProjectionMatrix());

  XMStoreFloat4x4(&m_constantBuffer.wvpMat, m * v * p);
}

void Model::PopulateCommandList(ComPtr<ID3D12GraphicsCommandList>& commandList, UINT8* cbAddress, D3D12_GPU_VIRTUAL_ADDRESS cbvAddress)
{
	commandList->SetGraphicsRoot32BitConstants(0, sizeof(ConstantBuffer) / sizeof(float), &m_constantBuffer, 0);

  m_mesh->PopulateCommandList(commandList, cbvAddress);
}

void Model::Release()
{
  if (m_mesh) m_mesh->Release();
}
