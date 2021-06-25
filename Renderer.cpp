#include "Renderer.h"

#include "Graphics.h"

bool Renderer::CreatePipelineState(ComPtr<ID3D12Device>& m_device, int width, int height)
{
  return true;
}

bool Renderer::LoadResources(ComPtr<ID3D12Device>& m_device, ComPtr<ID3D12GraphicsCommandList>& commandList, ComPtr<ID3D12CommandAllocator>& commandAllocator, int width, int height)
{
  return true;
}

bool Renderer::PopulateCommandList(
  ComPtr<ID3D12GraphicsCommandList>& commandList,
  ComPtr<ID3D12CommandAllocator>& commandAllocator,
  CD3DX12_CPU_DESCRIPTOR_HANDLE& rtvHandle,
  ComPtr<ID3D12Resource>& renderTarget,
  int frameIndex
)
{
  if (FAILED(commandAllocator->Reset())) return false;
  if (FAILED(commandList->Reset(commandAllocator.Get(), nullptr))) return false;

  commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(renderTarget.Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

  commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

  const float clearColor[] = { 0.502f, 0.729f, 0.141f, 1.0f };
  commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

  commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(renderTarget.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

  if (FAILED(commandList->Close())) return false;

  return true;
}

void Renderer::Update(int frameIndex)
{
}

void Renderer::Release() {}

//bool Renderer::Intersects(BoundingVolume& cameraBounds, XMFLOAT3& resolution)
//{
//  return false;
//}
