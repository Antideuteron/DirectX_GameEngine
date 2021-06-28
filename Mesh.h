#pragma once

#include "ObjLoader.h"
#include "TextureLoader.h"

class Mesh
{
public:
  static bool GetMesh(std::string object, std::string texture, Mesh*& mesh);
  ~Mesh(void) noexcept = default;

  void LoadResources(ComPtr<ID3D12Device>& device, ComPtr<ID3D12GraphicsCommandList>& commandList);
  void Update(int frameIndex);
  void PopulateCommandList(ComPtr<ID3D12GraphicsCommandList>& commandList, D3D12_GPU_VIRTUAL_ADDRESS cbvAddress);
  void Release();

  inline const std::string& ObjectFileName(void) const noexcept { return m_filename; }
  inline const std::wstring& TextureFileName(void) const noexcept { return m_texturename; }

  virtual bool CreateIndexBuffer(ComPtr<ID3D12Device>& device, ComPtr<ID3D12GraphicsCommandList>& commandList, DWORD*, int);
  virtual bool CreateVertexBuffer(ComPtr<ID3D12Device>& device, ComPtr<ID3D12GraphicsCommandList>& commandList, Vertex* iList, int vertexBufferSize);
  virtual bool LoadTexture(ComPtr<ID3D12Device>& device, ComPtr<ID3D12GraphicsCommandList>& commandList);

  const int VertexCount(void) const noexcept { return m_vertexCount; }
  const Vertex* Vertices(void) const noexcept { return m_Verticies; }

private:
  static std::map<std::string, Mesh> cache;

  Mesh(void) noexcept = default;
  Mesh(std::string filename, std::string texture) : m_filename(filename), m_texturename(texture.begin(), texture.end()) {}

  bool loaded = false;
  int instances = 0;

  ComPtr<ID3D12Resource> m_indexBuffer;
  ComPtr<ID3D12Resource> m_indexBufferUpload;
  D3D12_INDEX_BUFFER_VIEW m_indexBufferView;

  ComPtr<ID3D12Resource> m_vertexBuffer;
  ComPtr<ID3D12Resource> m_vertexBufferUpload;
  D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;

  ComPtr<ID3D12Resource> m_textureBuffer;
  ComPtr<ID3D12Resource> m_textureBufferUploadHeap;

  ComPtr<ID3D12DescriptorHeap> m_shaderResourceViewDescriptorHeap;

  int m_indexCount = 0;
  int m_vertexCount = 0;
  Vertex* m_Verticies;
  std::string m_filename;
  std::wstring m_texturename;

};
