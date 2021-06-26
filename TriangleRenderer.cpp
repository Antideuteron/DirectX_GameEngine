#include "TriangleRenderer.h"

#include <cmath>

bool TriangleRenderer::CreatePipelineState(ComPtr<ID3D12Device>& device, int width, int height)
{
  m_viewport = CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height));
  m_scissorRect = CD3DX12_RECT(0, 0, LONG_MAX, LONG_MAX);

  if (!CreateRootSignature(device)) return false;
  
  ComPtr<ID3DBlob> vertexShader;
  ComPtr<ID3DBlob> pixelShader;
  
  if (!CompileShaders(vertexShader, "main", pixelShader, "main")) return false;

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
  psoDesc.DepthStencilState.DepthEnable = FALSE;
  psoDesc.DepthStencilState.StencilEnable = FALSE;
  psoDesc.SampleMask = UINT_MAX;
  psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
  psoDesc.NumRenderTargets = 1;
  psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
  psoDesc.SampleDesc.Count = 1;

  if (FAILED(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(m_pipelineState.GetAddressOf()))))
  {
    Log::Error(L"CreateGraphicsPipelineState failed");

    return false;
  }

  Log::Info(L"CreatePipelineState succeeded");

	return true;
}

bool TriangleRenderer::LoadResources(ComPtr<ID3D12Device>& device, ComPtr<ID3D12GraphicsCommandList>& commandList, ComPtr<ID3D12CommandAllocator>& commandAllocator, int width, int height)
{
  const float aspect = static_cast<float>(width) / static_cast<float>(height);

  Vertex triangleVertices[] =
  {
    { {  0.5f, -0.5f * aspect, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f } },
    { { -0.5f, -0.5f * aspect, 0.0f }, { 1.0f, 1.0f, 0.0f, 1.0f }, { 0.0f, 0.0f } },
    { {  0.0f,  0.5f * aspect, 0.0f }, { 0.0f, 0.0f, 1.0f, 1.0f }, { 0.5f, 1.0f } },
  };

  if (FAILED(commandAllocator->Reset())) { Log::Error(L"commandAllocator::Reset failed"); return false; }

  commandList->Reset(commandAllocator.Get(), m_pipelineState.Get());

  if (!CreateVertexBuffer(device, commandList, triangleVertices, sizeof(triangleVertices))) return false;

  if (FAILED(commandList->Close())) { Log::Error(L"commandList::Close failed"); return false; }

  Log::Info(L"TriangleRenderer::LoadResources succeeded");

  return true;
}

bool TriangleRenderer::PopulateCommandList(ComPtr<ID3D12GraphicsCommandList>& commandList, ComPtr<ID3D12CommandAllocator>& commandAllocator, CD3DX12_CPU_DESCRIPTOR_HANDLE& rtvHandle, ComPtr<ID3D12Resource>& renderTarget, int frameIndex)
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

  commandList->SetGraphicsRoot32BitConstants(0, sizeof(ConstantBuffer) / 4, &m_constantBuffer, 0);

  commandList->DrawInstanced(3, 1, 0, 0);

  // Indicate that the back buffer will now be used to present.
  commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(renderTarget.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

  if (FAILED(commandList->Close())) { Log::Error(L"commandList::Close failed"); return false; }

	return true;
}

void TriangleRenderer::Release()
{
  m_inter.Reset();
  m_vertexBuffer.Reset();
  m_pipelineState.Reset();
  m_rootSignature.Reset();

  Renderer::Release();
}

bool TriangleRenderer::CreateVertexBuffer(ComPtr<ID3D12Device>& device, ComPtr<ID3D12GraphicsCommandList>& commandList, Vertex* vertexList, int vertexBufferSize)
{
  if (m_inter) m_inter->Release();

  if (FAILED(device->CreateCommittedResource(
    &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
    D3D12_HEAP_FLAG_NONE,
    &CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize),
    D3D12_RESOURCE_STATE_COPY_DEST,
    nullptr,
    IID_PPV_ARGS(m_vertexBuffer.GetAddressOf()))))
  {
    Log::Error(L"DestBuffer CreateCommittedResource failed");

    return false;
  }

  if (FAILED(device->CreateCommittedResource(
    &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
    D3D12_HEAP_FLAG_NONE,
    &CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize),
    D3D12_RESOURCE_STATE_GENERIC_READ,
    nullptr,
    IID_PPV_ARGS(m_inter.GetAddressOf()))))
  {
    Log::Error(L"InterBuffer CreateCommittedResource failed");

    return false;
  }

  D3D12_SUBRESOURCE_DATA subresourceData = {};
  subresourceData.pData = (void*)vertexList;
  subresourceData.RowPitch = vertexBufferSize;
  subresourceData.SlicePitch = subresourceData.RowPitch;

  UpdateSubresources(commandList.Get(), m_vertexBuffer.Get(), m_inter.Get(), 0, 0, 1, &subresourceData);
  commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_vertexBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER));

  m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
  m_vertexBufferView.StrideInBytes = sizeof(Vertex);
  m_vertexBufferView.SizeInBytes = vertexBufferSize;

  Log::Info(L"CreateVertexBuffer succeeded");

  return true;
}

bool TriangleRenderer::CreateRootSignature(ComPtr<ID3D12Device>& device)
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

bool TriangleRenderer::CompileShaders(ComPtr<ID3DBlob>& vertexShader, LPCSTR entryPointVertexShader, ComPtr<ID3D10Blob>& pixelShader, LPCSTR entryPointPixelShader)
{
#if defined(_DEBUG) && 0
    // Enable better shader debugging with the graphics debugging tools.
  UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
  UINT compileFlags = 0;
#endif

  ID3DBlob* errorBlob = nullptr;

  std::vector<D3D_SHADER_MACRO> macros;

  for (const auto macro : SHADER_MACROS)
  {
    macros.push_back(macro);
  }
  macros.push_back({ nullptr, nullptr });

  if (FAILED(D3DCompileFromFile(L"VertexShader.hlsl", macros.data(), nullptr, entryPointVertexShader, "vs_5_0", compileFlags, 0, &vertexShader, &errorBlob)))
  {
    if (errorBlob)
    {
      Log::Info((char*)errorBlob->GetBufferPointer());
      errorBlob->Release();
    }

    Log::Error("VertexShader.hlsl - D3DCompileFromFile failed");

    return false;
  }
  
  if (FAILED(D3DCompileFromFile(L"PixelShader.hlsl", macros.data(), nullptr, entryPointPixelShader, "ps_5_0", compileFlags, 0, &pixelShader, nullptr)))
  {
    if (errorBlob)
    {
      Log::Info((char*)errorBlob->GetBufferPointer());
      errorBlob->Release();
    }

    Log::Error("PixelShader.hlsl - D3DCompileFromFile failed");

    return false;
  }

  Log::Info(L"CompileShaders succeeded");

  return true;
}
