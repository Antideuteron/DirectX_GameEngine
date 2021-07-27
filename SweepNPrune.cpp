#include "SweepNPrune.hpp"

std::vector<BoundingVolume*> broad(const std::vector<BoundingVolume*>& models) noexcept
{
	std::vector<BoundingVolume*> sphereIntersections;

	for (const auto model : models) if (model->m_SphereTransformed.Intersects(Camera::Body())) sphereIntersections.push_back(model);

	return sphereIntersections;
}

uint32_t narrow(const std::vector<BoundingVolume*>& models) noexcept
{
	if (models.size() == 0) return 0;

	uint32_t collisions = 0u;

	for (const auto model : models) if (model->m_OBBTransformed.Intersects(Camera::Body())) ++collisions;

	return collisions;
}
