#include "Mesh.h"

std::map<std::string, Mesh> Mesh::cache;

bool Mesh::GetMesh(std::string object, std::string texture, Mesh*& mesh)
{
  auto& it = cache.find(object);

  if (it != cache.end())
  {
    mesh = &it->second;

    return true;
  }

  mesh = new Mesh(object, texture);
  const auto entry = cache.insert({ object, *mesh });

  return entry.second;
}

void Mesh::LoadResources(ComPtr<ID3D12Device>& device, ComPtr<ID3D12GraphicsCommandList>& commandList)
{
  if (loaded) return;

  DWORD* indexList;

  ObjLoader::Load(m_filename, m_Verticies, m_vertexCount, indexList, m_indexCount);

  if (m_vertexCount && m_indexCount)
  {
    CreateVertexBuffer(device, commandList, m_Verticies, m_vertexCount * sizeof(Vertex));
    CreateIndexBuffer(device, commandList, indexList, m_indexCount * sizeof(DWORD));
    LoadTexture(device, commandList);
  }

  delete[] indexList;

  loaded = true;
}

void Mesh::Update(int frameIndex)
{
}

void Mesh::PopulateCommandList(ComPtr<ID3D12GraphicsCommandList>& commandList, D3D12_GPU_VIRTUAL_ADDRESS cbvAddress)
{
  //// set the descriptor heap
  //ID3D12DescriptorHeap* descriptorHeaps[] = { m_shaderResourceViewDescriptorHeap.Get() };
  //commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
  //// set the descriptor table to the descriptor heap (parameter 1, as constant buffer root descriptor is parameter index 0)
  //commandList->SetGraphicsRootDescriptorTable(1, m_shaderResourceViewDescriptorHeap->GetGPUDescriptorHandleForHeapStart());

  commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
  commandList->IASetIndexBuffer(&m_indexBufferView);

  commandList->DrawIndexedInstanced(m_indexCount, 1, 0, 0, 0);
}

void Mesh::Release()
{
  --instances;

  if (instances > 0) return;

  m_indexBuffer.Reset();
  m_indexBufferUpload.Reset();
  m_vertexBuffer.Reset();
  m_vertexBufferUpload.Reset();
  m_textureBuffer.Reset();

  for (auto it = cache.begin(); it != cache.end(); ++it)
  {
    if (&it->second == this)
    {
      cache.erase(it);

      return;
    }
  }
}

bool Mesh::CreateIndexBuffer(ComPtr<ID3D12Device>& device, ComPtr<ID3D12GraphicsCommandList>& commandList, DWORD* indexList, int indexBufferSize)
{
  if (m_indexBufferUpload) m_indexBufferUpload->Release();

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
    IID_PPV_ARGS(m_indexBufferUpload.GetAddressOf()))))
  {
    Log::Error(L"InterBuffer CreateCommittedResource failed");

    return false;
  }

  D3D12_SUBRESOURCE_DATA subresourceData = {};

  subresourceData.pData = (void*)indexList;
  subresourceData.RowPitch = indexBufferSize;
  subresourceData.SlicePitch = subresourceData.RowPitch;

  UpdateSubresources(commandList.Get(), m_indexBuffer.Get(), m_indexBufferUpload.Get(), 0, 0, 1, &subresourceData);

  commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_indexBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER));

  m_indexBufferView.BufferLocation = m_indexBuffer->GetGPUVirtualAddress();
  m_indexBufferView.SizeInBytes = indexBufferSize;
  m_indexBufferView.Format = DXGI_FORMAT_R32_UINT;

  Log::Info(L"CreateIndexBuffer succeeded");

  return true;
}

bool Mesh::CreateVertexBuffer(ComPtr<ID3D12Device>& device, ComPtr<ID3D12GraphicsCommandList>& commandList, Vertex* vertexList, int vertexBufferSize)
{
  if (m_vertexBufferUpload) m_vertexBufferUpload->Release();

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
    IID_PPV_ARGS(m_vertexBufferUpload.GetAddressOf()))))
  {
    Log::Error(L"InterBuffer CreateCommittedResource failed");

    return false;
  }

  D3D12_SUBRESOURCE_DATA subresourceData = {};

  subresourceData.pData = (void*)vertexList;
  subresourceData.RowPitch = vertexBufferSize;
  subresourceData.SlicePitch = subresourceData.RowPitch;

  UpdateSubresources(commandList.Get(), m_vertexBuffer.Get(), m_vertexBufferUpload.Get(), 0, 0, 1, &subresourceData);

  commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_vertexBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER));

  m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
  m_vertexBufferView.StrideInBytes = sizeof(Vertex);
  m_vertexBufferView.SizeInBytes = vertexBufferSize;

  Log::Info(L"CreateVertexBuffer succeeded");

  return true;
}

bool Mesh::LoadTexture(ComPtr<ID3D12Device>& device, ComPtr<ID3D12GraphicsCommandList>& commandList)
{
  // Load the image from file
  D3D12_RESOURCE_DESC textureDesc;
  int imageBytesPerRow;
  BYTE* imageData;

  const auto imageSize = TextureLoader::LoadImageDataFromFile(
    &imageData,
    textureDesc,
    m_texturename.c_str(),
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
        IID_PPV_ARGS(&m_textureBuffer)
      )
    )
    )
  {
    return false;
  }
  m_textureBuffer->SetName(L"Texture Buffer Resource Heap");

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
        IID_PPV_ARGS(&m_textureBufferUploadHeap))
    )
  )
  {
    Log::Info(L"Failed to create committed resource texture upload heap");

    return false;
  }
  m_textureBufferUploadHeap->SetName(L"Texture Buffer Upload Resource Heap");

  // store vertex buffer in upload heap
  D3D12_SUBRESOURCE_DATA textureData = {};

  textureData.pData = &imageData[0]; // pointer to our image data
  textureData.RowPitch = imageBytesPerRow; // size of all our triangle vertex data
  textureData.SlicePitch = imageBytesPerRow * textureDesc.Height; // also the size of our triangle vertex data

  // Now we copy the upload buffer contents to the default heap
  UpdateSubresources(
    commandList.Get(),
    m_textureBuffer.Get(),
    m_textureBufferUploadHeap.Get(),
    0, 0, 1,
    &textureData
  );

  // transition the texture default heap to a pixel shader resource (we will be sampling from this heap in the pixel shader to get the color of pixels)
  commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_textureBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));

  // create the descriptor heap that will store our srv
  D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};

  heapDesc.NumDescriptors = 1;
  heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
  heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

  if (FAILED(device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(m_shaderResourceViewDescriptorHeap.GetAddressOf()))))
  {
    m_textureBufferUploadHeap->SetName(L"Failed to create texture descriptor heap");

    return false;
  }

  // now we create a shader resource view (descriptor that points to the texture and describes it)
  D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};

  srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
  srvDesc.Format = textureDesc.Format;
  srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
  srvDesc.Texture2D.MipLevels = 1;

  device->CreateShaderResourceView(m_textureBuffer.Get(), &srvDesc, m_shaderResourceViewDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

  // we are done with image data now that we've uploaded it to the gpu, so free it up
  delete imageData;

  return true;
}
