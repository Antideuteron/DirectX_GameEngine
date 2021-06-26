#pragma once

#include "Mesh.h"

class Model
{
public:
  Model(const std::string& object, const std::string& texture, XMFLOAT3 position, XMFLOAT4 rotation);
  ~Model(void) = default;

  void LoadResources(ComPtr<ID3D12Device>& device, ComPtr<ID3D12GraphicsCommandList>& commandList);
  void PopulateCommandList(ComPtr<ID3D12GraphicsCommandList>& commandList, UINT8* cbAddress, D3D12_GPU_VIRTUAL_ADDRESS cbvAddress);
  void Release();

private:
  XMFLOAT3 m_position;
  XMFLOAT4 m_rotation;
  Mesh* m_mesh;
  ConstantBuffer m_constantBuffer;
};
