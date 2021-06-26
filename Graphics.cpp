#include "Graphics.h"

#include "Mouse.hpp"
#include "Camera.hpp"
#include "Keyboard.hpp"

#include "DeviceManager.h"
#include "LevelRenderer.h"
#include "ImageRenderer.h"

bool Graphics::Init(HWND hwnd, int width, int height)
{
  if (m_loaded)
  {
    Release();
    
    m_loaded = false;
    m_resize = true;
  }

  if (!DeviceManager::CreateDevice(m_device, m_factory)) return false;
  if (!DeviceManager::CreateComandQueue(m_device, m_commandQueue, m_commandAllocator, m_commandList, m_fence, m_fenceEvent, m_fenceValue)) return false;
  if (!DeviceManager::CreateSwapChain(hwnd, m_factory, m_commandQueue, width, height, m_swapChain, m_frameIndex)) return false;
  if (!DeviceManager::CreateRenderTargets(m_device, m_swapChain, m_renderTargets.data(), m_rtvHeap, m_rtvDescriptorSize)) return false;

  m_renderer = new LevelRenderer();

  Camera::Init(width, height);

  if (!m_renderer->CreatePipelineState(m_device, width, height)) return false;
  if (!m_renderer->LoadResources(m_device, m_commandList, m_commandAllocator, width, height)) return false;

  ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };

  m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

  const UINT64 fence = m_fenceValue;

  if (FAILED(m_commandQueue->Signal(m_fence.Get(), fence))) return false;
  m_fenceValue++;

  if (!Sync()) return false;

  m_loaded = true;
  m_resize = false;

  return true;
}

bool Graphics::Render()
{
  if (!m_loaded) return m_resize;

  Camera::Update();
  m_renderer->Update(m_frameIndex);

  CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart(), m_frameIndex, m_rtvDescriptorSize);

  if (!m_renderer->PopulateCommandList(m_commandList, m_commandAllocator, rtvHandle, m_renderTargets[m_frameIndex], m_frameIndex))
  {
    m_loaded = false;

    return false;
  }

  ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
  m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

  // Present the frame.
  if (FAILED(m_swapChain->Present(1, 0)))
  {
    m_device->GetDeviceRemovedReason();

    m_loaded = false;

    return false;
  }

  return Sync();
}

bool Graphics::Sync()
{
  const UINT64 fence = m_fenceValue;
  HRESULT hr = m_commandQueue->Signal(m_fence.Get(), fence);

  if (FAILED(hr)) return false;

  m_fenceValue++;

  if (m_fence->GetCompletedValue() < fence)
  {
    hr = m_fence->SetEventOnCompletion(fence, m_fenceEvent);

    if (FAILED(hr)) return false;

    WaitForSingleObject(m_fenceEvent, INFINITE);
  }

  m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

  return true;
}

void Graphics::Release()
{
  if (m_renderer) {
    m_renderer->Release();
    delete(m_renderer);
  }
  // internal release
  for (int i = 0; i < sizeof(m_renderTargets) / sizeof(m_renderTargets[0]); i++) {
    if (m_renderTargets[i]) m_renderTargets[i].Reset();
  }
  if (m_fence) m_fence.Reset();
  if (m_rtvHeap) m_rtvHeap.Reset();
  if (m_commandAllocator)m_commandAllocator.Reset();
  if (m_commandList)m_commandList.Reset();
  if (m_commandQueue)m_commandQueue.Reset();

  if (m_swapChain)m_swapChain.Reset();
  if (m_factory)m_factory.Reset();
  if (m_device) m_device.Reset();

#ifdef _DEBUG
  ComPtr<IDXGIDebug1> debugController;
  if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(debugController.GetAddressOf()))))
  {
    debugController->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_FLAGS(DXGI_DEBUG_RLO_SUMMARY | DXGI_DEBUG_RLO_IGNORE_INTERNAL));
  }
#endif // DEBUG
}

LRESULT Graphics::WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  switch (message)
  {
  case WM_DESTROY:
    Application::Stop();
    PostQuitMessage(0);

    return 0;

  case WM_SYSKEYDOWN:
  case WM_KEYDOWN:
  {
    unsigned int scancode = (lParam >> 16) & 0xff;
    unsigned int extended = (lParam >> 24) & 0x1;

    if (extended)
    {
      if (scancode != 0x45)
      {
        scancode |= 0xE000;
      }
    }
    else
    {
      if (scancode == 0x45)
      {
        scancode = 0xE11D45;
      }
      else if (scancode == 0x54)
      {
        scancode = 0xE037;
      }
    }

    Keyboard::KeyDown(scancode);

    return 0;
  }
  case WM_KEYUP:
  case WM_SYSKEYUP:
  {
    unsigned int scancode = (lParam >> 16) & 0xff;
    unsigned int extended = (lParam >> 24) & 0x1;

    if (extended)
    {
      if (scancode != 0x45)
      {
        scancode |= 0xE000;
      }
    }
    else
    {
      if (scancode == 0x45)
      {
        scancode = 0xE11D45;
      }
      else if (scancode == 0x54)
      {
        scancode = 0xE037;
      }
    }

    Keyboard::KeyUp(scancode);

    return 0;
  }

  case WM_PAINT:
  {
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd, &ps);
    HBRUSH brush = CreateSolidBrush(RGB(0, 0, 0));

    FillRect(hdc, &ps.rcPaint, brush);
    EndPaint(hwnd, &ps);

    return 0;
  }

  case WM_SIZE:
  {
    Graphics* g = Application::GetGraphics();

    if (!g) break;

    g->Sync();

    RECT clientRect = {};
    ::GetClientRect(hwnd, &clientRect);

    int width = clientRect.right - clientRect.left;
    int height = clientRect.bottom - clientRect.top;

    g->Init(Application::WindowHandle(), width, height);

    return 0;
  }

  case WM_INPUT:
    Mouse::Update(lParam);
    break;

  case WM_QUIT: return wParam;
  }

  return DefWindowProc(hwnd, message, wParam, lParam);
}
