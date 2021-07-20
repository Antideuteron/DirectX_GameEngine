#pragma once

#include "DepthQuadRenderer.h"

#include "Model.h"

class LevelRenderer : public DepthQuadRenderer
{
public:
  virtual bool CreatePipelineState(ComPtr<ID3D12Device>&, int, int);
  virtual bool LoadResources(ComPtr<ID3D12Device>&, ComPtr<ID3D12GraphicsCommandList>&, ComPtr<ID3D12CommandAllocator>& commandAllocator, int, int);
  virtual bool PopulateCommandList(ComPtr<ID3D12GraphicsCommandList>& commandList, ComPtr<ID3D12CommandAllocator>& commandAllocator, CD3DX12_CPU_DESCRIPTOR_HANDLE& rtvHandle, ComPtr<ID3D12Resource>& renderTarget, int);
  virtual void Update(int frameIndex);
  virtual void Release();

protected:
  virtual bool CreateRootSignature(ComPtr<ID3D12Device>& device);

  const int m_constantBufferAlignedSize = (sizeof(ConstantBuffer) + 255) & ~255;

  ComPtr<ID3D12DescriptorHeap> mainDescriptorHeap;

  std::vector<Model*> m_models;
  std::vector<std::vector<int>> levelLayout;

private:
  void Render(ComPtr<ID3D12GraphicsCommandList>& commandList) noexcept;

};

