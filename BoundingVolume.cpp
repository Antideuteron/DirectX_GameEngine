#include "BoundingVolume.h"

constexpr const float max_float = std::numeric_limits<float>::max();
constexpr const float min_float = std::numeric_limits<float>::min();

BoundingVolume::BoundingVolume(std::vector<Vertex>& vertices) noexcept
{
	// initializes with extremes
	XMFLOAT3 min = { max_float, max_float, max_float };
	XMFLOAT3 max = { min_float, min_float, min_float };

	for (const auto vertex : vertices) // I think this is magic
	{
		// check component-wise for lower bound
		if (vertex.Position.x < min.x) min.x = vertex.Position.x;
		if (vertex.Position.y < min.y) min.y = vertex.Position.y;
		if (vertex.Position.z < min.z) min.z = vertex.Position.z;

		// check component-wise for upper bound
		if (vertex.Position.x > max.x) max.x = vertex.Position.x;
		if (vertex.Position.y > max.y) max.y = vertex.Position.y;
		if (vertex.Position.z > max.z) max.z = vertex.Position.z;
	}

	// with this min/max we can determine the AABB and the sphere

	//For the box, set:
	m_AABB = { min, max };

	//For the sphere Version 1 (bigger sphere):
	{ // determining the sphere needs some extra bits of work

		// first: determining the center point

		// Save min and max as XMVector
		XMVECTOR vmin = XMLoadFloat3(&min);
		XMVECTOR vmax = XMLoadFloat3(&max);

		// Centerpoint of the object can be found on the midpoint of the space diagonal 
		XMStoreFloat4(&m_Sphere.CenterRadius, XMVectorLerp(vmin, vmax, 0.5f));

		// second: determine radius of our box
		const auto vdist = ( vmax - vmin ) / 2;
		XMFLOAT3 dist;
		
		XMStoreFloat3(&dist, vdist);
		
		// not a beauty at all but straight forward for educational purpose
		m_Sphere.CenterRadius.w = std::sqrtf(dist.x * dist.x + dist.y * dist.y + dist.z * dist.z);

		// setting radius in m_SphereTransformed 'cause it will stay constant
		m_SphereTransformed.CenterRadius.w = m_Sphere.CenterRadius.w;
	}

	//For the sphere Version 2.0 (smaller sphere):
	{ 
		// first: determining the center point
		XMVECTOR center = { 0.0f, 0.0f, 0.0f };
		float n = 0;

		for (const auto vertex : vertices) // Summ up all vertex vectors
		{
			n += 1; // Count number of vertices
			center.m128_f32[0] += vertex.Position.x;
			center.m128_f32[1] += vertex.Position.y;
			center.m128_f32[2] += vertex.Position.z;
		}
		//Divide by n to find center
		center.m128_f32[0] = center.m128_f32[0] / n;
		center.m128_f32[1] = center.m128_f32[1] / n;
		center.m128_f32[2] = center.m128_f32[2] / n;

		float max_dist = 0;
		XMVECTOR dist = { 0.0f, 0.0f, 0.0f };
		for (const auto vertex : vertices) // Calculate maximum distance to centerpoint
		{
			dist.m128_f32[0] = center.m128_f32[0] - vertex.Position.x;
			dist.m128_f32[1] = center.m128_f32[1] - vertex.Position.y;
			dist.m128_f32[2] = center.m128_f32[2] - vertex.Position.z;

			float curr_dist = dist.m128_f32[0] * dist.m128_f32[0] + dist.m128_f32[1] * dist.m128_f32[1] + dist.m128_f32[2] * dist.m128_f32[2];
			if (curr_dist > max_dist) {
				max_dist = curr_dist;
			}

		}

		m_Sphere.CenterRadius.x = center.m128_f32[0];
		m_Sphere.CenterRadius.y = center.m128_f32[1];
		m_Sphere.CenterRadius.z = center.m128_f32[2];
		m_Sphere.CenterRadius.w = sqrtf(max_dist);

		// setting radius in m_SphereTransformed 'cause it will stay constant
		m_SphereTransformed.CenterRadius.w = m_Sphere.CenterRadius.w;
	}
}

bool BoundingVolume::Intersects(BoundingVolume* other, XMFLOAT3& resolution) noexcept
{
	return false;
}

void BoundingVolume::Update(XMFLOAT3* position, XMFLOAT4* rotation) noexcept
{
	// make some static constants to use every time
	static const XMFLOAT4 scale = { 1.0f, 1.0f, 1.0f, 1.0f };
	static const auto vsca = XMLoadFloat4(&scale);
	static const XMFLOAT4 origin = { 0.0f, 0.0f, 0.0f, 1.0f };
	static const auto vori = XMLoadFloat4(&origin);

	// load floats to vectors
	const auto vpos = XMLoadFloat3(position);
	const auto vrot = XMLoadFloat4(rotation);

	{// first: OBB
		// create affine transformation matrix
		const auto transform = XMMatrixAffineTransformation(vsca, vori, vrot, vpos);

		const auto obb_max = XMLoadFloat4(&m_OBB.Max);
		const auto obb_min = XMLoadFloat4(&m_OBB.Min);

		XMStoreFloat4(&m_OBBTransformed.Max, XMVector4Transform(obb_max, transform));
		XMStoreFloat4(&m_OBBTransformed.Min, XMVector4Transform(obb_min, transform));
	}

	{// second: AABB
		// Add new position to m_AABB into m_AABBTransformed for Max
		m_AABBTransformed.Max.x = m_AABB.Max.x + position->x;
		m_AABBTransformed.Max.y = m_AABB.Max.y + position->y;
		m_AABBTransformed.Max.z = m_AABB.Max.z + position->z;

		// Add new position to m_AABB into m_AABBTransformed for Min
		m_AABBTransformed.Min.x = m_AABB.Min.x + position->x;
		m_AABBTransformed.Min.y = m_AABB.Min.y + position->y;
		m_AABBTransformed.Min.z = m_AABB.Min.z + position->z;

		rangeAABB.xbegin = m_AABBTransformed.Min.x;
		rangeAABB.ybegin = m_AABBTransformed.Min.y;
		rangeAABB.zbegin = m_AABBTransformed.Min.z;
		rangeAABB.xend   = m_AABBTransformed.Max.x;
		rangeAABB.yend   = m_AABBTransformed.Max.y;
		rangeAABB.zend   = m_AABBTransformed.Max.z;
	}

	{// third: Sphere
		// Add new position to m_Sphere into m_SphereTransformed for CenterRadius[x,y,z]
		m_SphereTransformed.CenterRadius.x = m_Sphere.CenterRadius.x + position->x;
		m_SphereTransformed.CenterRadius.y = m_Sphere.CenterRadius.y + position->y;
		m_SphereTransformed.CenterRadius.z = m_Sphere.CenterRadius.z + position->z;

		rangeSphere.xbegin = m_SphereTransformed.CenterRadius.x - m_SphereTransformed.CenterRadius.w * 0.5f;
		rangeSphere.ybegin = m_SphereTransformed.CenterRadius.y - m_SphereTransformed.CenterRadius.w * 0.5f;
		rangeSphere.zbegin = m_SphereTransformed.CenterRadius.z - m_SphereTransformed.CenterRadius.w * 0.5f;
		rangeSphere.xend   = m_SphereTransformed.CenterRadius.x - m_SphereTransformed.CenterRadius.w * 0.5f;
		rangeSphere.yend   = m_SphereTransformed.CenterRadius.y - m_SphereTransformed.CenterRadius.w * 0.5f;
		rangeSphere.zend   = m_SphereTransformed.CenterRadius.z - m_SphereTransformed.CenterRadius.w * 0.5f;
	}
}

XMFLOAT3 BoundingVolume::insectCheck(const OBB& other) noexcept
{
	return XMFLOAT3();
}

XMFLOAT3 BoundingVolume::insectCheck(const AABB& other) noexcept
{
	return XMFLOAT3();
}

XMFLOAT3 BoundingVolume::insectCheck(const Sphere& other) noexcept
{
	const auto combinedRadius = m_Sphere.CenterRadius.w + other.CenterRadius.w;
	// determine the distance vector between both spheres
	const auto dist = XMFLOAT3{
		m_Sphere.CenterRadius.x - other.CenterRadius.x,
		m_Sphere.CenterRadius.y - other.CenterRadius.y,
		m_Sphere.CenterRadius.z - other.CenterRadius.z
	};

	// not a beauty but as previous in update; straight forward for educational purpose
	const auto sphereDistance = std::sqrtf(dist.x * dist.x + dist.y * dist.y + dist.z * dist.z);

	// calculating remaining distance of any
	const auto distance = sphereDistance - combinedRadius;

	if (distance == 0.0f)
	{ // this is a touch at this moment
		// here could be a resolution done BUTT we don't know speed of spheres, therefor wait for next simulation tick to resolve
	}
	else if (distance < 0.0f)
	{ // this is an intersection
		// here we could result in different resolution vectors to resolve depending on the desired effects
		// since we don't have different masses for the objects aswell and no movement on the y axis

		// a possible plan could be:

		// create a vector with half intersection distance (halfDist, 0.0f, halfDist)
		return { distance * 0.5f, 0.0f, distance * 0.5f }; // return it

		// resolution code in model update:
		// for every model
		//  determine 'movement' direction vector
		//  create response vector with cross product of direction and UP vector
		//  move model along that new cross-product-direction vector with the amount of resolution vector
	}

	// no intersection
	return { 0.0f, 0.0f, 0.0f };
}
