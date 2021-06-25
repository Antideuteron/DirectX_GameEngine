#include "DepthQuadRenderer.h"

bool DepthQuadRenderer::CreatePipelineState(ComPtr<ID3D12Device>& device, int width, int height)
{
  m_viewport = CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height));
  m_scissorRect = CD3DX12_RECT(0, 0, LONG_MAX, LONG_MAX);

  if (!CreateRootSignature(device)) return false;

  ComPtr<ID3DBlob> vertexShader;
  ComPtr<ID3DBlob> pixelShader;

  if (!CompileShaders(vertexShader, "mainCB", pixelShader, "main")) return false;

  // Define the vertex input layout.
  D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
  {
    { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 28, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
  };

  // Describe and create the graphics pipeline state object (PSO).
  D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};

  psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
  psoDesc.pRootSignature = m_rootSignature.Get();
  psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader.Get());
  psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShader.Get());
  psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
  psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
  psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
  psoDesc.SampleMask = UINT_MAX;
  psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
  psoDesc.NumRenderTargets = 1;
  psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
  psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
  psoDesc.SampleDesc.Count = 1;

  if (FAILED(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(m_pipelineState.GetAddressOf()))))
  {
    Log::Error(L"CreateGraphicsPipelineState failed");

    return false;
  }

  Log::Info(L"CreatePipelineState succeeded");

  return true;
}

bool DepthQuadRenderer::LoadResources(ComPtr<ID3D12Device>& device, ComPtr<ID3D12GraphicsCommandList>& commandList, ComPtr<ID3D12CommandAllocator>& commandAllocator, int width, int height)
{
  const float aspect = static_cast<float>(width) / static_cast<float>(height);

  Vertex vertices[] =
  {
    { { -0.5f, 0.5f * aspect, 0.7f }, { 0.0f, 1.0f, 0.0f, 1.0f }, { 1.0f, 1.0f } },
    { {  0.0f, 0.0f * aspect, 0.7f }, { 0.0f, 1.0f, 0.0f, 1.0f }, { 1.0f, 1.0f } },
    { { -0.5f, 0.0f * aspect, 0.7f }, { 0.0f, 1.0f, 0.0f, 1.0f }, { 1.0f, 1.0f } },
    { {  0.0f, 0.5f * aspect, 0.7f }, { 0.0f, 1.0f, 0.0f, 1.0f }, { 1.0f, 1.0f } },

    { { -0.25f,  0.25f * aspect, 0.5f }, { 0.0f, 0.0f, 1.0f, 1.0f }, { 1.0f, 1.0f } },
    { {  0.25f, -0.25f * aspect, 0.5f }, { 0.0f, 0.0f, 1.0f, 1.0f }, { 1.0f, 1.0f } },
    { { -0.25f, -0.25f * aspect, 0.5f }, { 0.0f, 0.0f, 1.0f, 1.0f }, { 1.0f, 1.0f } },
    { {  0.25f,  0.25f * aspect, 0.5f }, { 0.0f, 0.0f, 1.0f, 1.0f }, { 1.0f, 1.0f } },
  };

  DWORD indices[] = { 0, 1, 2, 0, 3, 1, 4, 5, 6, 4, 7, 5 };

  if (FAILED(commandAllocator->Reset())) { Log::Error(L"commandAllocator::Reset failed"); return false; }

  commandList->Reset(commandAllocator.Get(), m_pipelineState.Get());

  if (!CreateVertexBuffer(device, commandList, vertices, sizeof(vertices))) return false;
  if (!CreateIndexBuffer(device, commandList, indices, sizeof(indices))) return false;
  if (!CreateDepthStencilBuffer(device, commandList, width, height)) return false;

  if (FAILED(commandList->Close())) { Log::Error(L"commandList::Close failed"); return false; }

  Log::Info(L"DepthQuadRenderer::LoadResources succeeded");

  return true;
}

bool DepthQuadRenderer::PopulateCommandList(ComPtr<ID3D12GraphicsCommandList>& commandList, ComPtr<ID3D12CommandAllocator>& commandAllocator, CD3DX12_CPU_DESCRIPTOR_HANDLE& rtvHandle, ComPtr<ID3D12Resource>& renderTarget, int frameIndex)
{
  if (FAILED(commandAllocator->Reset())) { Log::Error(L"commandAllocator::Reset failed"); return false; }

  commandList->Reset(commandAllocator.Get(), m_pipelineState.Get());

  commandList->SetGraphicsRootSignature(m_rootSignature.Get());
  commandList->RSSetViewports(1, &m_viewport);
  commandList->RSSetScissorRects(1, &m_scissorRect);

  // Indicate that the back buffer will be used as a render target.

  commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(renderTarget.Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

  // Record commands.
  auto dsvHandle = m_depthStencilDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
  commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

  //const float clearColor[] = { 0.502f, 0.729f, 0.141f, 1.0f };
  const float clearColor[] = { 0.08f, 0.08f, 0.08f, 1.0f };
  commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
  commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

  commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
  commandList->IASetIndexBuffer(&m_indexBufferView);

  commandList->SetGraphicsRoot32BitConstants(0, sizeof(ConstantBuffer) / sizeof(float), &m_constantBuffer, 0);

  commandList->DrawIndexedInstanced(12, 1, 0, 0, 0);

  // Indicate that the back buffer will now be used to present.
  commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(renderTarget.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

  if (FAILED(commandList->Close())) { Log::Error(L"commandList::Close failed"); return false; }

  return true;
}

void DepthQuadRenderer::Release()
{
	m_depthStencilBuffer.Reset();
	m_depthStencilDescriptorHeap.Reset();

	QuadRenderer::Release();
}

void DepthQuadRenderer::Update(int frameIndex)
{
  angle = std::fmodf(angle + 0.2f, 360.0f);

  XMStoreFloat4x4(&m_constantBuffer.wvpMat, XMMatrixRotationZ(XMConvertToRadians(angle)));
}

bool DepthQuadRenderer::CreateRootSignature(ComPtr<ID3D12Device>& device)
{
  CD3DX12_ROOT_PARAMETER1 rootParameters;
  rootParameters.InitAsConstants(sizeof(ConstantBuffer) / sizeof(float), 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);

  CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
  rootSignatureDesc.Init_1_1(1, &rootParameters, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

  ComPtr<ID3DBlob> signature;
  ComPtr<ID3DBlob> error;

  if (FAILED(D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error))) {
    if (error)
    {
      Log::Info((char*)error->GetBufferPointer());
      error->Release();
    }

    Log::Error(L"SerializeRootSignature failed");

    return false;
  }

  if (FAILED(device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(m_rootSignature.GetAddressOf())))) {
    Log::Error(L"CreateRootSignature failed");

    return false;
  }

  Log::Info(L"CreateRootSignature succeeded");

  return true;
}

bool DepthQuadRenderer::CreateDepthStencilBuffer(ComPtr<ID3D12Device>& device, ComPtr<ID3D12GraphicsCommandList>& commandList, int width, int height)
{
  // create a depth stencil descriptor heap so we can get a pointer to the depth stencil buffer
  D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
  dsvHeapDesc.NumDescriptors = 1;
  dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
  dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

  if (FAILED(device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(m_depthStencilDescriptorHeap.GetAddressOf()))))
  {
    Log::Info("DepthStencil Descriptor Heap creation failed");

    return false;
  }

  Log::Info("DepthStencil Descriptor Heap successfully created");

  D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilDesc = {};
  depthStencilDesc.Format = DXGI_FORMAT_D32_FLOAT;
  depthStencilDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
  depthStencilDesc.Flags = D3D12_DSV_FLAG_NONE;

  D3D12_CLEAR_VALUE depthOptimizedClearValue = {};
  depthOptimizedClearValue.Format = DXGI_FORMAT_D32_FLOAT;
  depthOptimizedClearValue.DepthStencil.Depth = 1.0f;
  depthOptimizedClearValue.DepthStencil.Stencil = 0;

  device->CreateCommittedResource(
    &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
    D3D12_HEAP_FLAG_NONE,
    &CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, width, height, 1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL),
    D3D12_RESOURCE_STATE_DEPTH_WRITE,
    &depthOptimizedClearValue,
    IID_PPV_ARGS(&m_depthStencilBuffer)
  );
  m_depthStencilDescriptorHeap->SetName(L"Depth/Stencil Resource Heap");

  device->CreateDepthStencilView(m_depthStencilBuffer.Get(), &depthStencilDesc, m_depthStencilDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

  Log::Info("DepthStencil Descriptor View successfully created");

  return true;
}
