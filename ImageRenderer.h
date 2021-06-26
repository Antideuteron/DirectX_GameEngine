#pragma once

#include "TextureLoader.h"
#include "DepthQuadRenderer.h"

class ImageRenderer :
  public DepthQuadRenderer
{
public:
  ImageRenderer(void) noexcept = default;
  ~ImageRenderer(void) noexcept = default;

  virtual bool CreatePipelineState(ComPtr<ID3D12Device>&, int width, int height);
  virtual bool LoadResources(ComPtr<ID3D12Device>&, ComPtr<ID3D12GraphicsCommandList>&, ComPtr<ID3D12CommandAllocator>& commandAllocator, int width, int height);

  virtual bool PopulateCommandList(ComPtr<ID3D12GraphicsCommandList>& commandList, ComPtr<ID3D12CommandAllocator>& commandAllocator, CD3DX12_CPU_DESCRIPTOR_HANDLE& rtvHandle, ComPtr<ID3D12Resource>& renderTarget, int frameIndex);
  virtual void Update(int frameIndex);

  virtual void Release();

protected:
  virtual bool CreateRootSignature(ComPtr<ID3D12Device>& device);
  virtual bool LoadTexture(ComPtr<ID3D12Device>& device, ComPtr<ID3D12GraphicsCommandList>& commandList);

  ComPtr<ID3D12Resource> textureBuffer; // the resource heap containing our texture

  ComPtr<ID3D12DescriptorHeap> mainDescriptorHeap;
  ComPtr<ID3D12Resource> textureBufferUploadHeap;
};