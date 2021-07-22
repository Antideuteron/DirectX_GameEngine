#include "DeviceManager.h"

bool DeviceManager::CreateDevice(ComPtr<ID3D12Device>& device, ComPtr<IDXGIFactory4>& factory)
{
  UINT dxgiFactoryFlags = 0;

#if defined(_DEBUG)
  {
    ComPtr<ID3D12Debug1> debugController;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(debugController.GetAddressOf()))))
    {
      debugController->EnableDebugLayer();

      dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
    }
  }
#endif

  if (FAILED(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(factory.GetAddressOf())))) return false;

  ComPtr<IDXGIAdapter1> pAdapterToUse = nullptr; // Zeiger auf ersten Hardware-Adapter
  ComPtr<IDXGIAdapter1> pWARP= nullptr; // Zeiger auf den WARP-Adapter

  {
    ComPtr<IDXGIAdapter1> pAdapter;

    for (UINT i = 0u; factory->EnumAdapters1(i, pAdapter.GetAddressOf()) != DXGI_ERROR_NOT_FOUND; ++i)
    {
      DXGI_ADAPTER_DESC1 desc;

      pAdapter->GetDesc1(&desc);

      if (desc.DeviceId == 0x8c)
      {
        pWARP = pAdapter;

        continue;
      }

      Log::Info(desc.Description);

      if (!pAdapterToUse) pAdapterToUse = pAdapter; // Der erste wird gesetzt
      else pAdapter->Release();
    }
  }

  const bool useWarp = pAdapterToUse && FAILED(D3D12CreateDevice( // Wenn erster vorhanden dann versuch ansonsten WARP
    pAdapterToUse.Get(),
    D3D_FEATURE_LEVEL_11_0,
    IID_PPV_ARGS(device.GetAddressOf())
  ));

  if (!pWARP) factory->EnumWarpAdapter(IID_PPV_ARGS(pWARP.GetAddressOf()));

  if (useWarp)
  {
    const bool failed = !FAILED(D3D12CreateDevice(pWARP.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(device.GetAddressOf())));

    pWARP->Release();

    return failed;
  }

  pWARP->Release();
  pAdapterToUse->Release();

  return true;
}

bool DeviceManager::CreateComandQueue(ComPtr<ID3D12Device>& device, ComPtr<ID3D12CommandQueue>& commandQueue, ComPtr<ID3D12CommandAllocator>& commandAllocator, ComPtr<ID3D12GraphicsCommandList>& commandList, ComPtr<ID3D12Fence>& fence, HANDLE& fenceEvent, UINT64& fenceValue)
{
  D3D12_COMMAND_QUEUE_DESC queueDesc = {};
  queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
  queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

  if (FAILED(device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(commandQueue.GetAddressOf())))) return false;

  if (FAILED(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(commandAllocator.GetAddressOf())))) return false;
  commandAllocator->SetName(L"main allocator");

  // Create the command list.
  if (FAILED(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator.Get(), nullptr, IID_PPV_ARGS(commandList.GetAddressOf())))) return false;

  if (FAILED(commandList->Close())) return false;

  // Create synchronization objects.
  {
    if (FAILED(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(fence.GetAddressOf())))) return false;
    fenceValue = 1;

    // Create an event handle to use for frame synchronization.
    fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (fenceEvent == nullptr)
    {
      if (FAILED(HRESULT_FROM_WIN32(GetLastError()))) return false;
    }
  }

  Log::Info("Created command stack");
  return true;
}

bool DeviceManager::CreateSwapChain(HWND& hwnd, ComPtr<IDXGIFactory4>& factory, ComPtr<ID3D12CommandQueue>& commandQueue, UINT width, UINT height, ComPtr<IDXGISwapChain3>& swapChain, UINT& frameIndex)
{
  DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};

  swapChainDesc.BufferCount = 2;
  swapChainDesc.Width = width;
  swapChainDesc.Height = height;
  swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
  swapChainDesc.SampleDesc.Count = 1;
  swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING; // TODO: set back to 0

  ComPtr<IDXGISwapChain1> swapChain1;

  if (FAILED(factory->CreateSwapChainForHwnd(commandQueue.Get(), hwnd, &swapChainDesc, nullptr, nullptr, &swapChain1))) return false;

  if (FAILED(swapChain1.As(&swapChain))) return false;

  Log::Info("Created Swap Chain");

  frameIndex = swapChain->GetCurrentBackBufferIndex();

  return true;
}

bool DeviceManager::CreateRenderTargets(ComPtr<ID3D12Device>& device, ComPtr<IDXGISwapChain3>& swapChain, ComPtr<ID3D12Resource> renderTargets[2], ComPtr<ID3D12DescriptorHeap>& rtvHeap, UINT& rtvDescriptorSize)
{
  D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};

  rtvHeapDesc.NumDescriptors = 2;
  rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
  rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

  if (FAILED(device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(rtvHeap.GetAddressOf())))) return false;

  rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

  CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtvHeap->GetCPUDescriptorHandleForHeapStart());

  for (UINT n = 0; n < 2; n++)
  {
    if (FAILED(swapChain->GetBuffer(n, IID_PPV_ARGS(renderTargets[n].GetAddressOf())))) return false;

    device->CreateRenderTargetView(renderTargets[n].Get(), nullptr, rtvHandle);

    rtvHandle.Offset(1, rtvDescriptorSize);
  }

  Log::Info("Created Render Targets");

  return true;
}
