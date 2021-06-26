#pragma once

#include "Renderer.h"

class Camera
{
public:
  static bool Init(const uint32_t width, const uint32_t height) noexcept;

  static void Update();

  static void Rotate(int, int);

  static XMFLOAT4X4 GetViewMatrix();
  static XMFLOAT4X4 GetProjectionMatrix();

private:
  Camera(void) noexcept = delete;
  ~Camera(void) noexcept = delete;

  static XMFLOAT4 m_position;
  static XMFLOAT4 m_target;
  static XMFLOAT4 m_up;

  static XMFLOAT4 m_rotation;
};