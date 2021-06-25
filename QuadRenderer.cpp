#include "stdafx.h"
#include "QuadRenderer.h"

bool QuadRenderer::LoadResources(ComPtr<ID3D12Device>& device, ComPtr<ID3D12GraphicsCommandList>& commandList, ComPtr<ID3D12CommandAllocator>& commandAllocator, int width, int height)
{
  const float aspect = static_cast<float>(width) / static_cast<float>(height);

  Vertex vertices[] =
  {
      { {  0.5f, -0.5f * aspect, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f } },
      { { -0.5f, -0.5f * aspect, 0.0f }, { 1.0f, 1.0f, 0.0f, 1.0f }, { 1.0f, 0.0f } },
      { {  0.5f,  0.5f * aspect, 0.0f }, { 0.0f, 1.0f, 1.0f, 1.0f }, { 1.0f, 1.0f } },
      { { -0.5f,  0.5f * aspect, 0.0f }, { 0.0f, 0.0f, 1.0f, 1.0f }, { 0.0f, 1.0f } },
  };

  DWORD indices[] = { 0, 1, 2, 1, 3, 2 };

  if (FAILED(commandAllocator->Reset())) { Log::Error(L"commandAllocator::Reset failed"); return false; }

  commandList->Reset(commandAllocator.Get(), m_pipelineState.Get());

  if (!CreateVertexBuffer(device, commandList, vertices, sizeof(vertices))) return false;
  if (!CreateIndexBuffer(device, commandList, indices, sizeof(indices))) return false;

  if (FAILED(commandList->Close())) { Log::Error(L"commandList::Close failed"); return false; }

  Log::Info(L"QuadRenderer::LoadResources succeeded");

  return true;
}

bool QuadRenderer::PopulateCommandList(ComPtr<ID3D12GraphicsCommandList>& commandList, ComPtr<ID3D12CommandAllocator>& commandAllocator, CD3DX12_CPU_DESCRIPTOR_HANDLE& rtvHandle, ComPtr<ID3D12Resource>& renderTarget, int frameIndex)
{
  if (FAILED(commandAllocator->Reset())) { Log::Error(L"commandAllocator::Reset failed"); return false; }

  commandList->Reset(commandAllocator.Get(), m_pipelineState.Get());

  commandList->SetGraphicsRootSignature(m_rootSignature.Get());
  commandList->RSSetViewports(1, &m_viewport);
  commandList->RSSetScissorRects(1, &m_scissorRect);

  // Indicate that the back buffer will be used as a render target.

  commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(renderTarget.Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

  // Record commands.
  commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

  const float clearColor[] = { 0.502f, 0.729f, 0.141f, 1.0f };
  commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

  commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
  commandList->IASetIndexBuffer(&m_indexBufferView);

  commandList->SetGraphicsRoot32BitConstants(0, sizeof(ConstantBuffer) / 4, &m_constantBuffer, 0);

  commandList->DrawIndexedInstanced(6, 1, 0, 0, 0);

  // Indicate that the back buffer will now be used to present.
  commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(renderTarget.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

  if (FAILED(commandList->Close())) { Log::Error(L"commandList::Close failed"); return false; }

  return true;
}

void QuadRenderer::Release()
{
  m_indexBuffer.Reset();
  m_interIndexBuffer.Reset();

  TriangleRenderer::Release();
}

bool QuadRenderer::CreateIndexBuffer(ComPtr<ID3D12Device>& device, ComPtr<ID3D12GraphicsCommandList>& commandList, DWORD* iList, int indexBufferSize)
{
  if (m_interIndexBuffer) m_interIndexBuffer->Release();

  if (FAILED(device->CreateCommittedResource(
    &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
    D3D12_HEAP_FLAG_NONE,
    &CD3DX12_RESOURCE_DESC::Buffer(indexBufferSize),
    D3D12_RESOURCE_STATE_COPY_DEST,
    nullptr,
    IID_PPV_ARGS(m_indexBuffer.GetAddressOf()))))
  {
    Log::Error(L"DestBuffer CreateCommittedResource failed");

    return false;
  }

  if (FAILED(device->CreateCommittedResource(
    &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
    D3D12_HEAP_FLAG_NONE,
    &CD3DX12_RESOURCE_DESC::Buffer(indexBufferSize),
    D3D12_RESOURCE_STATE_GENERIC_READ,
    nullptr,
    IID_PPV_ARGS(m_interIndexBuffer.GetAddressOf()))))
  {
    Log::Error(L"InterBuffer CreateCommittedResource failed");

    return false;
  }

  D3D12_SUBRESOURCE_DATA subresourceData = {};
  subresourceData.pData = (void*)iList;
  subresourceData.RowPitch = indexBufferSize;
  subresourceData.SlicePitch = subresourceData.RowPitch;

  UpdateSubresources(commandList.Get(), m_indexBuffer.Get(), m_interIndexBuffer.Get(), 0, 0, 1, &subresourceData);
  commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_indexBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER));

  m_indexBufferView.BufferLocation = m_indexBuffer->GetGPUVirtualAddress();
  m_indexBufferView.SizeInBytes = indexBufferSize;
  m_indexBufferView.Format = DXGI_FORMAT_R32_UINT;

  Log::Info(L"QuadRenderer::CreateIndexBuffer succeeded");

  return true;
}
