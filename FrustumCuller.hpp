#pragma once

#include "Model.h"

BoundingVolumeTestType CullingUpdate(void) noexcept;
void FrustumCull(const std::vector<Model*>& models, std::vector<Model*>&) noexcept;
