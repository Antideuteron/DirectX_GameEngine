#include "FrustumCuller.hpp"

#include "Camera.hpp"

void FrustumCull(const std::vector<Model*>& models, std::vector<Model*>& toRender) noexcept
{
	const auto BVTT = BoundingVolume::BVTT();
	BoundingVolume::BVTT() = BoundingVolumeTestType::Sphere;

	for (auto& model : models)
	{
		XMFLOAT3 resolution;

		if (Camera::Frustum().Intersects(model->m_BoundingVolume, resolution)) toRender.push_back(model);
	}
	BoundingVolume::BVTT() = BVTT;
}
