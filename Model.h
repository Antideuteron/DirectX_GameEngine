#pragma once

#include "Mesh.h"
#include "BoundingVolume.h"

class Model
{
public:
  Model(const std::string& object, const std::string& texture, XMFLOAT3 position, XMFLOAT4 rotation, const bool solid = true);
  ~Model(void) = default;

  void LoadResources(ComPtr<ID3D12Device>& device, ComPtr<ID3D12GraphicsCommandList>& commandList);
  void Update(int frameIndex);
  void PopulateCommandList(ComPtr<ID3D12GraphicsCommandList>& commandList, UINT8* cbAddress, D3D12_GPU_VIRTUAL_ADDRESS cbvAddress);
  void Release();

  inline const bool isSolid(void) const noexcept { return m_Solid; }

private:
  static constexpr int m_constantBufferAlignedSize = (sizeof(ConstantBuffer) + 255) & ~255;

  XMFLOAT3 m_position;
  XMFLOAT4 m_rotation;
  Mesh* m_mesh;
  ConstantBuffer m_constantBuffer;
  const bool m_Solid;

public:
  BoundingVolume m_BoundingVolume;

};
