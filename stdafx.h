#pragma once

#include <SDKDDKVer.h>

#include <cmath>
#include <cctype>
#include <cstdio>
#include <cwchar>
#include <climits>
#include <cstdlib>
#include <cstring>

#include <chrono>
#include <limits>
#include <memory>
#include <utility>
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <streambuf>
#include <functional>

#include <map>
#include <set>
#include <array>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <WinUser.h>

#undef min
#undef max

#include <d3d12.h>
#include <dxgi1_4.h>
#include <initguid.h>
#include <dxgidebug.h>
#include <D3Dcompiler.h>
#include <DirectXMath.h>
#include <directxcollision.h>
#include <directxpackedvector.h>

#include <d3dx12.h>
#include <wincodec.h>

#include <wrl.h>
#include <shellapi.h>

#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")
#pragma comment(lib,"dxguid.lib")
#pragma comment(lib,"d3dcompiler.lib")
#pragma comment(lib,"windowscodecs.lib")

using std::vector;
using Microsoft::WRL::ComPtr;
using namespace DirectX;

#include "Log.h"
#include "resource.h"
#include "Application.h"

#ifdef _DEBUG
#define CHECK_RESULT(res) {const HRESULT _ = (res); if(FAILED(_)) { Log::Error("An error occured"); __debugbreak(); }}
#else
#define CHECK_RESULT(res) {(res)}
#endif

struct ConstantBuffer
{
  XMFLOAT4 colorMultiplier = { 1.0f, 1.0f, 1.0f, 1.0f };
  XMFLOAT4X4 wvpMat;
};

struct Vertex
{
  XMFLOAT3 Position;
  XMFLOAT4 Color;
  XMFLOAT2 TexCoord;

  Vertex(void) noexcept = default;
  Vertex(const XMFLOAT3& position, const XMFLOAT4& color, const XMFLOAT2& texCoord) noexcept : Position(position), Color(color), TexCoord(texCoord) {}
  inline Vertex(const XMVECTOR& vector) noexcept;
};

Vertex::Vertex(const XMVECTOR& vector) noexcept
{
  XMFLOAT3 point;

  XMStoreFloat3(&point, vector);

  Position = { point.x, point.y, point.z };
}

inline float wrap_angle(const float angle) noexcept
{
  if (angle < -XM_PI) return angle + XM_PI + XM_PI;
  if (angle > XM_PI) return angle - XM_PI - XM_PI;
  return angle;
}
