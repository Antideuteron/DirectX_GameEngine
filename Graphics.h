#pragma once

#include <array>

#include "Renderer.h"

class Graphics
{
public:
  Graphics() = default;
  ~Graphics() { if (m_renderer) delete m_renderer; };

  bool Init(HWND, int windowWidth, int windowHeight);
  bool Render();
  void Release();
  Renderer& GetRenderer() { return *m_renderer; };

  static LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

private:
  bool Sync();

  bool m_loaded = false;
  bool m_resize = false;
  ComPtr<ID3D12Device> m_device;
  std::array<ComPtr<ID3D12Resource>, 2> m_renderTargets;
  ComPtr<IDXGISwapChain3> m_swapChain;

  ComPtr<ID3D12CommandQueue> m_commandQueue;
  ComPtr<ID3D12CommandAllocator> m_commandAllocator;
  ComPtr<ID3D12GraphicsCommandList> m_commandList;
  HANDLE m_fenceEvent;
  ComPtr<ID3D12Fence> m_fence;
  UINT64 m_fenceValue = 0;

  ComPtr<IDXGIFactory4> m_factory;
  ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
  UINT m_rtvDescriptorSize;
  UINT m_frameIndex;

  Renderer* m_renderer;
};