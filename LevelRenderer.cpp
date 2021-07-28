#include "LevelRenderer.h"

#include "Camera.hpp"
#include "LevelLoader.h"

bool LevelRenderer::CreatePipelineState(ComPtr<ID3D12Device>& device, int width, int height)
{
  m_viewport = CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height));
  m_scissorRect = CD3DX12_RECT(0, 0, LONG_MAX, LONG_MAX);

  if (!CreateRootSignature(device)) return false;

  D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
  heapDesc.NumDescriptors = 1;
  heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
  heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
  if (FAILED(device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&mainDescriptorHeap))))
  {
    return false;
  }

  ComPtr<ID3DBlob> vertexShader;
  ComPtr<ID3DBlob> pixelShader;

  SHADER_MACROS.push_back({ "TEXTURE", "0" });

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
  psoDesc.RasterizerState.FrontCounterClockwise = true;
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

  Log::Info(L"GraphicsPipelineState successfully created");

  return true;
}

bool LevelRenderer::LoadResources(ComPtr<ID3D12Device>& device, ComPtr<ID3D12GraphicsCommandList>& commandList, ComPtr<ID3D12CommandAllocator>& commandAllocator, int width, int height)
{
  const XMFLOAT4 rotation = { 0.0f, 0.0f, 0.0f, 1.0f };
  XMFLOAT4 degree45;
  XMStoreFloat4(&degree45, XMQuaternionRotationAxis({ 0, 1, 0 }, XMConvertToRadians(45.0f)));
  const auto level = LevelLoader::Load("level1.txt");

  XMFLOAT3 position = { 4.0f, 0.0f, 3.0f };

  for (const auto row : level)
  {
    for (const int cell : row)
    {
      switch (cell)
      {
      case 0: m_models.push_back(new Model("wall.obj", "wall.png", position, rotation)); break;
      case 1: m_models.push_back(new Model("floor.obj", "floor.png", position, rotation, false)); break;
      case 2:
        m_models.push_back(new Model("floor.obj", "floor.png", position, rotation, false));
        m_models.push_back(new Model("barrier.obj", "barrier.png", position, degree45));
        break;
      default:break;
      }

      position.z -= 2.0f;
    }

    position.x -= 2.0f;
    position.z = 3.0f;
  }

  commandList->Reset(commandAllocator.Get(), m_pipelineState.Get());

  if (!CreateDepthStencilBuffer(device, commandList, width, height)) return false;

  for (auto& model : m_models) model->LoadResources(device, commandList);

  commandList->Close();

  return true;
}

bool LevelRenderer::PopulateCommandList(ComPtr<ID3D12GraphicsCommandList>& commandList, ComPtr<ID3D12CommandAllocator>& commandAllocator, CD3DX12_CPU_DESCRIPTOR_HANDLE& rtvHandle, ComPtr<ID3D12Resource>& renderTarget, int frameIndex)
{
  if (FAILED(commandAllocator->Reset())) return false;
  if (FAILED(commandList->Reset(commandAllocator.Get(), nullptr))) return false;

  commandList->SetPipelineState(m_pipelineState.Get());
  commandList->SetGraphicsRootSignature(m_rootSignature.Get());
  commandList->RSSetViewports(1, &m_viewport);
  commandList->RSSetScissorRects(1, &m_scissorRect);

  const auto transe = CD3DX12_RESOURCE_BARRIER::Transition(renderTarget.Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
  commandList->ResourceBarrier(1, &transe);

  const auto dsvHandle = m_depthStencilDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
  commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

  ID3D12DescriptorHeap* descriptorHeaps[] = { mainDescriptorHeap.Get() };
  commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
  // set the descriptor table to the descriptor heap (parameter 1, as constant buffer root descriptor is parameter index 0)
  commandList->SetGraphicsRootDescriptorTable(1, mainDescriptorHeap->GetGPUDescriptorHandleForHeapStart());

  const float clearColor[] = { 0.08f, 0.08f, 0.08f, 1.0f };
  commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
  commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

  Render(commandList);

  const auto transe2 = CD3DX12_RESOURCE_BARRIER::Transition(renderTarget.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
  commandList->ResourceBarrier(1, &transe2);

  if (FAILED(commandList->Close())) return false;

  return true;
}

void LevelRenderer::Update(int frameIndex)
{
  std::vector<BoundingVolume*> solids;
  for (auto& model : m_models)
  {
    model->Update(frameIndex);

    if (model->isSolid()) solids.push_back(&model->m_BoundingVolume);
  }

  if (BoundingVolume::SweepNPrune(solids))
  {
    Log::Info(L"Sweep&Prune Narrow Phase COLLISION");
    Camera::m_Position.x -= Camera::Translation().x;
    Camera::m_Position.z -= Camera::Translation().z;
  }
}

void LevelRenderer::Release()
{
  for (auto& model : m_models) model->Release();
}

bool LevelRenderer::CreateRootSignature(ComPtr<ID3D12Device>& device)
{
  // create a descriptor range (descriptor table) and fill it out
  // this is a range of descriptors inside a descriptor heap
  D3D12_DESCRIPTOR_RANGE1  descriptorTableRanges[1]; // only one range right now
  descriptorTableRanges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV; // this is a range of shader resource views (descriptors)
  descriptorTableRanges[0].NumDescriptors = 1; // we only have one texture right now, so the range is only 1
  descriptorTableRanges[0].BaseShaderRegister = 0; // start index of the shader registers in the range
  descriptorTableRanges[0].RegisterSpace = 0; // space 0. can usually be zero
  descriptorTableRanges[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND; // this appends the range to the end of the root signature descriptor tables

  // create a root parameter for the root descriptor and fill it out
  CD3DX12_ROOT_PARAMETER1  rootParameters[2] = { {}, {} }; // two root parameters

  rootParameters[0].InitAsConstants(sizeof(ConstantBuffer) / sizeof(float), 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
  rootParameters[1].InitAsDescriptorTable(1, descriptorTableRanges, D3D12_SHADER_VISIBILITY_PIXEL);

  // create a static sampler
  D3D12_STATIC_SAMPLER_DESC sampler = {};
  sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
  sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
  sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
  sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
  sampler.MipLODBias = 0;
  sampler.MaxAnisotropy = 0;
  sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
  sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
  sampler.MinLOD = 0.0f;
  sampler.MaxLOD = D3D12_FLOAT32_MAX;
  sampler.ShaderRegister = 0;
  sampler.RegisterSpace = 0;
  sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

  CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
  rootSignatureDesc.Init_1_1(2, rootParameters, 1, &sampler, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

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

void LevelRenderer::Render(ComPtr<ID3D12GraphicsCommandList>& commandList) noexcept
{
  std::vector<Model*> renderables;
  const auto start = std::chrono::system_clock::now();
  BoundingVolume::FrustumCull(m_models, renderables);
  const auto end = std::chrono::system_clock::now();
  const std::chrono::duration<double> diff = (end - start);

  Log::Info((std::wstringstream() << L"Culling: " << diff.count() * 1000.0 << "ms - " << renderables.size() << " of " << m_models.size() << " models visible").str());

  for (auto& model : renderables) model->PopulateCommandList(commandList, nullptr, 0);
}
