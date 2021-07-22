#pragma once

#include "Renderer.h"
#include "BoundingVolume.h"

/*
  TODO Camera Klasse neu aufsetzen!!!
*/

class Camera
{
public:
  static bool Init(const uint32_t width, const uint32_t height) noexcept;

  static void Update();

  static void Rotate(int, int);
  static void Translate(XMFLOAT3& translation) noexcept;

  static XMFLOAT4X4 GetViewMatrix();
  static XMFLOAT4X4 GetProjectionMatrix();

  static inline BoundingVolume& Frustum(void) noexcept { return m_Frustum; }

private:
  Camera(void) noexcept = delete;
  ~Camera(void) noexcept = delete;

  static XMFLOAT3 m_Position;
  static XMFLOAT4 m_Rotation;

  static float speed;
  static float m_YAW, m_PITCH;

  static BoundingVolume m_Frustum;

  static std::array<Vertex, 8> m_FrustumPoints;

};