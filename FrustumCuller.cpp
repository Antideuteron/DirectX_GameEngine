#include "FrustumCuller.hpp"

#include "Camera.hpp"
#include "Keyboard.hpp"

static BoundingVolumeTestType s_Type = BoundingVolumeTestType::AABB;

BoundingVolumeTestType CullingUpdate(void) noexcept
{
	if      (Keyboard::IsPressed(sc_1)) return BoundingVolumeTestType::Sphere;
	else if (Keyboard::IsPressed(sc_2)) return BoundingVolumeTestType::AABB;
	else if (Keyboard::IsPressed(sc_3)) return BoundingVolumeTestType::OBB;
	
	return s_Type;
}

void FrustumCull(const std::vector<Model*>& models, std::vector<Model*>& toRender) noexcept
{
	s_Type = CullingUpdate();

	const auto BVTT = BoundingVolume::BVTT();
	BoundingVolume::BVTT() = s_Type;

	for (auto& model : models)
	{
		XMFLOAT3 resolution;

		if (Camera::Frustum().Intersects(&model->m_BoundingVolume, resolution)) toRender.push_back(model);
	}

	BoundingVolume::BVTT() = BVTT;
}
