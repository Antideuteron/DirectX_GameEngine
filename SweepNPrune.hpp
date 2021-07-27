#pragma once

#include "Camera.hpp"
#include "BoundingVolume.h"

std::vector<BoundingVolume*> broad(const std::vector<BoundingVolume*>& models) noexcept;
uint32_t narrow(const std::vector<BoundingVolume*>& models) noexcept;
