#pragma once

class TextureLoader
{
public:
  static int LoadImageDataFromFile(BYTE** imageData, D3D12_RESOURCE_DESC& resourceDescription, LPCWSTR filename, int& bytesPerRow);

private:
  TextureLoader(void) noexcept = default;
  ~TextureLoader(void) noexcept = default;


  static DXGI_FORMAT GetDXGIFormatFromWICFormat(WICPixelFormatGUID& wicFormatGUID);
  static WICPixelFormatGUID GetConvertToWICFormat(WICPixelFormatGUID& wicFormatGUID);
  static int GetDXGIFormatBitsPerPixel(DXGI_FORMAT& dxgiFormat);
};