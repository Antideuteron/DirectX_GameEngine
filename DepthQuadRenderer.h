#pragma once

#include "QuadRenderer.h"

class DepthQuadRenderer : public QuadRenderer
{
public:
  DepthQuadRenderer(void) noexcept = default;
  virtual ~DepthQuadRenderer(void) noexcept = default;

  virtual bool CreatePipelineState(ComPtr<ID3D12Device>& device, int width, int height);
  virtual bool LoadResources(ComPtr<ID3D12Device>& device, ComPtr<ID3D12GraphicsCommandList>& commandList, ComPtr<ID3D12CommandAllocator>& commandAllocator, int width, int height);

  virtual bool PopulateCommandList(ComPtr<ID3D12GraphicsCommandList>& commandList, ComPtr<ID3D12CommandAllocator>& commandAllocator, CD3DX12_CPU_DESCRIPTOR_HANDLE& rtvHandle, ComPtr<ID3D12Resource>& renderTarget, int frameIndex);

  virtual void Release();

  virtual void Update(int frameIndex);

protected:
  ComPtr<ID3D12Resource> m_depthStencilBuffer;
  ComPtr<ID3D12DescriptorHeap> m_depthStencilDescriptorHeap;

  virtual bool CreateRootSignature(ComPtr<ID3D12Device>& device);
  virtual bool CreateDepthStencilBuffer(ComPtr<ID3D12Device>& device, ComPtr<ID3D12GraphicsCommandList>& commandList, int width, int height);

private:
  float angle = 0.0f;

};