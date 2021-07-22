#include "BoundingVolume.h"

constexpr const float max_float = std::numeric_limits<float>::max();
constexpr const float min_float = std::numeric_limits<float>::min();

BoundingVolumeTestType BoundingVolume::TestType = BoundingVolumeTestType::Sphere;

BoundingVolume::BoundingVolume(std::vector<Vertex>& vertices) noexcept
{
	BoundingBox::CreateFromPoints(m_AABB, vertices.size(), (XMFLOAT3*)vertices.data(), sizeof(Vertex));
	BoundingSphere::CreateFromPoints(m_Sphere, vertices.size(), (XMFLOAT3*)vertices.data(), sizeof(Vertex));
	BoundingOrientedBox::CreateFromPoints(m_OBB, vertices.size(), (XMFLOAT3*)vertices.data(), sizeof(Vertex));

	//// initializes with extremes
	//XMFLOAT3 min = { max_float, max_float, max_float };
	//XMFLOAT3 max = { min_float, min_float, min_float };

	//for (const auto vertex : vertices) // I think this is magic
	//{
	//	// check component-wise for lower bound
	//	if (vertex.Position.x < min.x) min.x = vertex.Position.x;
	//	if (vertex.Position.y < min.y) min.y = vertex.Position.y;
	//	if (vertex.Position.z < min.z) min.z = vertex.Position.z;

	//	// check component-wise for upper bound
	//	if (vertex.Position.x > max.x) max.x = vertex.Position.x;
	//	if (vertex.Position.y > max.y) max.y = vertex.Position.y;
	//	if (vertex.Position.z > max.z) max.z = vertex.Position.z;
	//}

	//// with this min/max we can determine the AABB and the sphere

	////For the box, set:
	//m_AABB = { min, max };

	////For the sphere Version 2.0 (smaller sphere):
	//{ 
	//	// first: determining the center point
	//	XMFLOAT3 center = { 0.0f, 0.0f, 0.0f };
	//	float n = 1.0f / static_cast<float>(vertices.size());

	//	for (const auto& vertex : vertices) // Sum up all vertex vectors
	//	{
	//		center.x += vertex.Position.x;
	//		center.y += vertex.Position.y;
	//		center.z += vertex.Position.z;
	//	}

	//	//Divide by n to find center
	//	center.x *= n;
	//	center.y *= n;
	//	center.z *= n;

	//	float max_dist = 0.0f;
	//	for (const auto& vertex : vertices) // Calculate maximum distance to center point
	//	{
	//		XMFLOAT3 dist = {
	//			center.x - vertex.Position.x,
	//			center.y - vertex.Position.y,
	//			center.z - vertex.Position.z
	//		};

	//		float curr_dist = dist.x * dist.x + dist.y * dist.y + dist.z * dist.z;

	//		if (curr_dist > max_dist) {
	//			max_dist = curr_dist;
	//		}
	//	}

	//	m_Sphere.CenterRadius.x = center.x;
	//	m_Sphere.CenterRadius.y = center.y;
	//	m_Sphere.CenterRadius.z = center.z;
	//	m_Sphere.CenterRadius.w = sqrtf(max_dist);

	//	// setting radius in m_SphereTransformed 'cause it will stay constant
	//	m_SphereTransformed.CenterRadius.w = m_Sphere.CenterRadius.w;
	//}
}

bool BoundingVolume::Intersects(BoundingVolume* other, XMFLOAT3& resolution) noexcept
{
	switch (TestType)
	{
		case BoundingVolumeTestType::Sphere:
			resolution = insectCheck(other->m_SphereTransformed);
			return XMVector3LengthSq(XMLoadFloat3(&resolution)).m128_f32[0] != 0.0f;
		case BoundingVolumeTestType::AABB:
			resolution = insectCheck(other->m_AABBTransformed);
			return XMVector3LengthSq(XMLoadFloat3(&resolution)).m128_f32[0] != 0.0f;
		case BoundingVolumeTestType::OBB:
			resolution = insectCheck(other->m_OBBTransformed);
			return XMVector3LengthSq(XMLoadFloat3(&resolution)).m128_f32[0] != 0.0f;
	}

	return false;
}

void BoundingVolume::Update(XMFLOAT3* position, XMFLOAT4* rotation) noexcept
{
	// make some static constants to use every time
	static const XMVECTOR scale = { 1.0f, 1.0f, 1.0f, 1.0f };
	static const XMVECTOR origin = { 0.0f, 0.0f, 0.0f, 1.0f };

	// load floats to vectors
	const auto vpos = XMLoadFloat3(position);
	const auto vrot = XMLoadFloat4(rotation);

	const auto transform = XMMatrixAffineTransformation(scale, origin, vrot, vpos);

	m_OBB.Transform(m_OBBTransformed, transform);
	m_AABB.Transform(m_AABBTransformed, transform);
	m_Sphere.Transform(m_SphereTransformed, transform);

	const auto extends = m_AABBTransformed.Extents;

	rangeAABB.xbegin = m_AABBTransformed.Center.x - extends.x;
	rangeAABB.ybegin = m_AABBTransformed.Center.y - extends.y;
	rangeAABB.zbegin = m_AABBTransformed.Center.z - extends.z;
	rangeAABB.xend   = m_AABBTransformed.Center.x + extends.x;
	rangeAABB.yend   = m_AABBTransformed.Center.y + extends.y;
	rangeAABB.zend   = m_AABBTransformed.Center.z + extends.z;
}

XMFLOAT3 BoundingVolume::insectCheck(const BoundingOrientedBox& other) noexcept
{
	return { 0.0f, 0.0f, 0.0f };
}

XMFLOAT3 BoundingVolume::insectCheck(const BoundingBox& other) noexcept
{
	if (m_AABBTransformed.Intersects(other))
	{
		//m_AABBTransformed.
	}

	return { 0.0f, 0.0f, 0.0f };
}

XMFLOAT3 BoundingVolume::insectCheck(const BoundingSphere& other) noexcept
{
	if (m_SphereTransformed.Intersects(other))
	{ // this is an intersection
		// here we could result in different resolution vectors to resolve depending on the desired effects
		// since we don't have different masses for the objects aswell and no movement on the y axis

		// a possible plan could be:

		// create a vector with half intersection distance (halfDist, 0.0f, halfDist)
		const auto combinedRadius = m_SphereTransformed.Radius + other.Radius;
		// determine the distance vector between both spheres
		const auto dist = XMFLOAT3{
			m_SphereTransformed.Center.x - other.Center.x,
			m_SphereTransformed.Center.y - other.Center.y,
			m_SphereTransformed.Center.z - other.Center.z
		};	// not a beauty but as previous in update; straight forward for educational purpose
		const auto sphereDistance = std::sqrtf(dist.x * dist.x + dist.y * dist.y + dist.z * dist.z);

		// calculating remaining distance of any
		const auto distance = (sphereDistance - combinedRadius) * 0.5f;

		return { distance, 0.0f, distance }; // return it

		// resolution code in model update:
		// for every model
		//  determine 'movement' direction vector
		//  create response vector with cross product of direction and UP vector
		//  move model along that new cross-product-direction vector with the amount of resolution vector
	}

	// no intersection
	return { 0.0f, 0.0f, 0.0f };
}
