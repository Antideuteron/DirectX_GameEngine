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
	m_AABB = { min, max };

	{ // determining the sphere need some extra bits of work
		// first: determining the center point
		XMVECTOR vmin = XMLoadFloat3(&min);
		XMVECTOR vmax = XMLoadFloat3(&max);

		XMStoreFloat4(&m_Sphere.CenterRadius, XMVectorLerp(vmin, vmax, 0.5f));

		// second: determine midpoint

		const auto vdist = vmax - vmin;
		XMFLOAT3 dist;
		
		XMStoreFloat3(&dist, vdist);
		
		// not a beauty at all but straight forward for educational purpose
		m_Sphere.CenterRadius.w = std::sqrtf(dist.x * dist.x + dist.y * dist.y + dist.z * dist.z);

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

	// create affine transformation matrix
	const auto transform = XMMatrixAffineTransformation(vsca, vori, vrot, vpos);

	{// first: OBB
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
	}

	{// third: Sphere
		// Add new position to m_Sphere into m_SphereTransformed for CenterRadius[x,y,z]
		m_SphereTransformed.CenterRadius.x = m_Sphere.CenterRadius.x + position->x;
		m_SphereTransformed.CenterRadius.y = m_Sphere.CenterRadius.y + position->y;
		m_SphereTransformed.CenterRadius.z = m_Sphere.CenterRadius.z + position->z;
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
		// return it
		
		return { distance * 0.5f, 0.0f, distance * 0.5f };

		// resolution code in model update:
		// for every model
		//  determine 'movement' direction vector
		//  create response vector with cross product of direction and UP vector
		//  move model along that new cross-product-direction vector with the amount of resolution vector
	}

	// no intersection
	return { 0.0f, 0.0f, 0.0f };
}
