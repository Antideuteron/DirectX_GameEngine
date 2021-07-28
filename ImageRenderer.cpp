#include "ImageRenderer.h"

#include "Camera.hpp"
#include "ObjLoader.h"

bool ImageRenderer::CreatePipelineState(ComPtr<ID3D12Device>& device, int width, int height)
{
  m_viewport = CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height));
  m_scissorRect = CD3DX12_RECT(0, 0, LONG_MAX, LONG_MAX);

  if (!CreateRootSignature(device)) return false;

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
  psoDesc.RasterizerState.FrontCounterClockwise = false;
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

bool ImageRenderer::LoadResources(ComPtr<ID3D12Device>& device, ComPtr<ID3D12GraphicsCommandList>& commandList, ComPtr<ID3D12CommandAllocator>& commandAllocator, int width, int height)
{
  int32_t vertexCount = 0;
  Vertex* vertex = nullptr;
  int32_t indexCount = 0;
  DWORD* index = nullptr;

  OBJLoader::Load("plane.obj", vertex, vertexCount, index, indexCount);

  if (FAILED(commandAllocator->Reset())) { Log::Error(L"commandAllocator::Reset failed"); return false; }

  commandList->Reset(commandAllocator.Get(), m_pipelineState.Get());

  if (!CreateVertexBuffer(device, commandList, vertex, sizeof(Vertex) * vertexCount)) return false;
  if (!CreateIndexBuffer(device, commandList, index, sizeof(DWORD) * indexCount)) return false;
  if (!CreateDepthStencilBuffer(device, commandList, width, height)) return false;
  if (!LoadTexture(device, commandList)) return false;

  if (FAILED(commandList->Close())) { Log::Error(L"commandList::Close failed"); return false; }

  Log::Info(L"ImageRenderer::LoadResources succeeded");

  return true;
}

bool ImageRenderer::PopulateCommandList(ComPtr<ID3D12GraphicsCommandList>& commandList, ComPtr<ID3D12CommandAllocator>& commandAllocator, CD3DX12_CPU_DESCRIPTOR_HANDLE& rtvHandle, ComPtr<ID3D12Resource>& renderTarget, int frameIndex)
{
  if (FAILED(commandAllocator->Reset())) { Log::Error(L"commandAllocator::Reset failed"); return false; }

  commandList->Reset(commandAllocator.Get(), m_pipelineState.Get());

  commandList->SetGraphicsRootSignature(m_rootSignature.Get());
  commandList->RSSetViewports(1, &m_viewport);
  commandList->RSSetScissorRects(1, &m_scissorRect);

  // Indicate that the back buffer will be used as a render target.

  const auto transe = CD3DX12_RESOURCE_BARRIER::Transition(renderTarget.Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
  commandList->ResourceBarrier(1, &transe);

  // Record commands.
  auto dsvHandle = m_depthStencilDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
  commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

  // set the descriptor heap
  ID3D12DescriptorHeap* descriptorHeaps[] = { mainDescriptorHeap.Get() };
  commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
  // set the descriptor table to the descriptor heap (parameter 1, as constant buffer root descriptor is parameter index 0)
  commandList->SetGraphicsRootDescriptorTable(1, mainDescriptorHeap->GetGPUDescriptorHandleForHeapStart());


  //const float clearColor[] = { 0.502f, 0.729f, 0.141f, 1.0f };
  const float clearColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
  commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
  commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

  commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
  commandList->IASetIndexBuffer(&m_indexBufferView);

  commandList->SetGraphicsRoot32BitConstants(0, sizeof(ConstantBuffer) / sizeof(float), &m_constantBuffer, 0);

  commandList->DrawIndexedInstanced(6, 1, 0, 0, 0);

  // Indicate that the back buffer will now be used to present.
  commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(renderTarget.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

  if (FAILED(commandList->Close())) { Log::Error(L"commandList::Close failed"); return false; }

  return true;
}

void ImageRenderer::Update(int frameIndex)
{
  const XMMATRIX m = XMMatrixIdentity();
  const XMMATRIX v = XMLoadFloat4x4(&Camera::GetViewMatrix());
  const XMMATRIX p = XMLoadFloat4x4(&Camera::GetProjectionMatrix());

  XMStoreFloat4x4(&m_constantBuffer.wvpMat, XMMatrixIdentity());
}

void ImageRenderer::Release()
{
  textureBuffer.Reset();

  mainDescriptorHeap.Reset();
  textureBufferUploadHeap.Reset();

  DepthQuadRenderer::Release();
}

bool ImageRenderer::CreateRootSignature(ComPtr<ID3D12Device>& device)
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
  CD3DX12_ROOT_PARAMETER1  rootParameters[2]; // two root parameters
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

bool ImageRenderer::LoadTexture(ComPtr<ID3D12Device>& device, ComPtr<ID3D12GraphicsCommandList>& commandList)
{
  // Load the image from file
  D3D12_RESOURCE_DESC textureDesc;
  int imageBytesPerRow;
  BYTE* imageData;
  auto imageSize = TextureLoader::LoadImageDataFromFile(
    &imageData,
    textureDesc,
    L"thm.png",
    imageBytesPerRow
  );

  // make sure we have data
  if (imageSize <= 0)
  {
    Log::Info("loading of texture failed");

    return false;
  }

  // create a default heap where the upload heap will copy its contents into (contents being the texture)
  if (
    FAILED(
      device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), // a default heap
        D3D12_HEAP_FLAG_NONE, // no flags
        &textureDesc, // the description of our texture
        D3D12_RESOURCE_STATE_COPY_DEST, // We will copy the texture from the upload heap to here, so we start it out in a copy dest state
        nullptr, // used for render targets and depth/stencil buffers
        IID_PPV_ARGS(&textureBuffer)
      )
    )
  )
  {
    return false;
  }
  textureBuffer->SetName(L"Texture Buffer Resource Heap");

  UINT64 textureUploadBufferSize;
  // this function gets the size an upload buffer needs to be to upload a texture to the gpu.
  // each row must be 256 byte aligned except for the last row, which can just be the size in bytes of the row
  // eg. textureUploadBufferSize = ((((width * numBytesPerPixel) + 255) & ~255) * (height - 1)) + (width * numBytesPerPixel);
  //textureUploadBufferSize = (((imageBytesPerRow + 255) & ~255) * (textureDesc.Height - 1)) + imageBytesPerRow;
  device->GetCopyableFootprints(&textureDesc, 0, 1, 0, nullptr, nullptr, nullptr, &textureUploadBufferSize);

  // now we create an upload heap to upload our texture to the GPU
  if (
    FAILED(
      device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), // upload heap
        D3D12_HEAP_FLAG_NONE, // no flags
        &CD3DX12_RESOURCE_DESC::Buffer(textureUploadBufferSize), // resource description for a buffer (storing the image data in this heap just to copy to the default heap)
        D3D12_RESOURCE_STATE_GENERIC_READ, // We will copy the contents from this heap to the default heap above
        nullptr,
        IID_PPV_ARGS(&textureBufferUploadHeap))
      )
    )
  {
    return false;
  }
  textureBufferUploadHeap->SetName(L"Texture Buffer Upload Resource Heap");

  // store vertex buffer in upload heap
  D3D12_SUBRESOURCE_DATA textureData = {};
  textureData.pData = &imageData[0]; // pointer to our image data
  textureData.RowPitch = imageBytesPerRow; // size of all our triangle vertex data
  textureData.SlicePitch = imageBytesPerRow * textureDesc.Height; // also the size of our triangle vertex data

  // Now we copy the upload buffer contents to the default heap
  UpdateSubresources(
    commandList.Get(),
    textureBuffer.Get(),
    textureBufferUploadHeap.Get(),
    0, 0, 1,
    &textureData
  );

  // transition the texture default heap to a pixel shader resource (we will be sampling from this heap in the pixel shader to get the color of pixels)
  commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(textureBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));

  // create the descriptor heap that will store our srv
  D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
  heapDesc.NumDescriptors = 1;
  heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
  heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
  
  if (FAILED(device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(mainDescriptorHeap.GetAddressOf()))))
  {
    return false;
  }

  // now we create a shader resource view (descriptor that points to the texture and describes it)
  D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
  srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
  srvDesc.Format = textureDesc.Format;
  srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
  srvDesc.Texture2D.MipLevels = 1;
  device->CreateShaderResourceView(textureBuffer.Get(), &srvDesc, mainDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

  // we are done with image data now that we've uploaded it to the gpu, so free it up
  delete imageData;

  return true;
}
