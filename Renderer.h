#pragma once

class Renderer
{
public:
  Renderer() = default;
  virtual ~Renderer() = default;

  virtual bool CreatePipelineState(ComPtr<ID3D12Device>&, int width, int height);
  virtual bool LoadResources(ComPtr<ID3D12Device>&, ComPtr<ID3D12GraphicsCommandList>&, ComPtr<ID3D12CommandAllocator>& commandAllocator, int width, int height);

  virtual bool PopulateCommandList(ComPtr<ID3D12GraphicsCommandList>& commandList, ComPtr<ID3D12CommandAllocator>& commandAllocator, CD3DX12_CPU_DESCRIPTOR_HANDLE& rtvHandle, ComPtr<ID3D12Resource>& renderTarget, int frameIndex);
  virtual void Update(int frameIndex);

  virtual void Release();
  //virtual bool Intersects(BoundingVolume& cameraBounds, XMFLOAT3& resolution);
};
