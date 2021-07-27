#include "BoundingVolume.h"

#include "Camera.hpp"

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

void BoundingVolume::SimpleCollisionCheck(const std::vector<BoundingVolume*>& models) noexcept
{
	const auto& player = Camera::Body();

	for (const auto& model : models)
	{
		auto resolution = model->insectCheckOBBSphere(player);

		if (resolution.x != 0.0f || resolution.z != 0.0f)
		{
			Log::Info(
				(std::wstringstream() << "( " << resolution.x << " | " << resolution.y << " | " << resolution.z << " )\n").str()
			);

			Camera::m_Position.x -= Camera::Translation().x;
			//Camera::m_Position.y -= Camera::Translation().y;
			Camera::m_Position.z -= Camera::Translation().z;
		}
	}
}

XMFLOAT3 BoundingVolume::insectCheck(const BoundingOrientedBox& other) const noexcept
{
	if (m_OBBTransformed.Intersects(other))
	{
		return { 1.0f, 0.0f, 1.0f };
	}

	return { 0.0f, 0.0f, 0.0f };
}

XMFLOAT3 BoundingVolume::insectCheck(const BoundingBox& other) const noexcept
{
	if (m_AABBTransformed.Intersects(other))
	{
		const auto tminx = m_AABBTransformed.Center.x - m_AABBTransformed.Extents.x;
		const auto tmaxx = m_AABBTransformed.Center.x + m_AABBTransformed.Extents.x;
		const auto tminz = m_AABBTransformed.Center.z - m_AABBTransformed.Extents.z;
		const auto tmaxz = m_AABBTransformed.Center.z + m_AABBTransformed.Extents.z;

		const auto ominx = other.Center.x - other.Extents.x;
		const auto omaxx = other.Center.x + other.Extents.x;
		const auto ominz = other.Center.z - other.Extents.z;
		const auto omaxz = other.Center.z + other.Extents.z;

		const float xmint = tminx - omaxx;
		const float xmino = ominx - tmaxx;
		const float zmint = tminz - omaxz;
		const float zmino = ominz - tmaxz;

		return { (xmint > xmino ? xmint : xmino) * 0.5f, 0.0f, (zmint > zmino ? zmint : zmino) * 0.5f };
	}

	return { 0.0f, 0.0f, 0.0f };
}

XMFLOAT3 BoundingVolume::insectCheckAABBSphere(const BoundingSphere& sphere) const noexcept // WTF
{
	XMVECTOR SphereCenter = XMLoadFloat3(&sphere.Center);
	XMVECTOR SphereRadius = XMVectorReplicatePtr(&sphere.Radius);

	XMVECTOR BoxCenter = XMLoadFloat3(&m_AABBTransformed.Center);
	XMVECTOR BoxExtents = XMLoadFloat3(&m_AABBTransformed.Extents);

	XMVECTOR BoxMin = XMVectorSubtract(BoxCenter, BoxExtents);
	XMVECTOR BoxMax = XMVectorAdd(BoxCenter, BoxExtents);

	// Find the distance to the nearest point on the box.
	// for each i in (x, y, z)
	// if (SphereCenter(i) < BoxMin(i)) d2 += (SphereCenter(i) - BoxMin(i)) ^ 2
	// else if (SphereCenter(i) > BoxMax(i)) d2 += (SphereCenter(i) - BoxMax(i)) ^ 2

	XMVECTOR d = XMVectorZero();

	// Compute d for each dimension.
	XMVECTOR LessThanMin = XMVectorLess(SphereCenter, BoxMin);
	XMVECTOR GreaterThanMax = XMVectorGreater(SphereCenter, BoxMax);

	XMVECTOR MinDelta = XMVectorSubtract(SphereCenter, BoxMin);
	XMVECTOR MaxDelta = XMVectorSubtract(SphereCenter, BoxMax);

	// Choose value for each dimension based on the comparison.
	d = XMVectorSelect(d, MinDelta, LessThanMin);
	d = XMVectorSelect(d, MaxDelta, GreaterThanMax);

	// Use a dot-product to square them and sum them together.
	XMVECTOR d2 = XMVector3Dot(d, d);

	XMFLOAT3 distance;
	XMStoreFloat3(&distance, XMVectorSubtract(d2, XMVectorMultiply(SphereRadius, SphereRadius)));

	if (XMVector3LessOrEqual(d2, XMVectorMultiply(SphereRadius, SphereRadius)))
	{
		return { sqrtf(fabsf(distance.x)), 0.0f, sqrtf(fabsf(distance.z)) };
	}

	return { 0.0f, 0.0f, 0.0f };
}

XMFLOAT3 BoundingVolume::insectCheckOBBSphere(const BoundingSphere& sphere) const noexcept
{
	XMVECTOR SphereCenter = XMLoadFloat3(&sphere.Center);
	XMVECTOR SphereRadius = XMVectorReplicatePtr(&sphere.Radius);

	XMVECTOR BoxCenter = XMLoadFloat3(&m_OBBTransformed.Center);
	XMVECTOR BoxExtents = XMLoadFloat3(&m_OBBTransformed.Extents);
	XMVECTOR BoxOrientation = XMLoadFloat4(&m_OBBTransformed.Orientation);

	// Transform the center of the sphere to be local to the box.
	// BoxMin = -BoxExtents
	// BoxMax = +BoxExtents
	SphereCenter = XMVector3InverseRotate(XMVectorSubtract(SphereCenter, BoxCenter), BoxOrientation);

	// Find the distance to the nearest point on the box.
	// for each i in (x, y, z)
	// if (SphereCenter(i) < BoxMin(i)) d2 += (SphereCenter(i) - BoxMin(i)) ^ 2
	// else if (SphereCenter(i) > BoxMax(i)) d2 += (SphereCenter(i) - BoxMax(i)) ^ 2

	XMVECTOR d = XMVectorZero();

	// Compute d for each dimension.
	XMVECTOR LessThanMin = XMVectorLess(SphereCenter, XMVectorNegate(BoxExtents));
	XMVECTOR GreaterThanMax = XMVectorGreater(SphereCenter, BoxExtents);

	XMVECTOR MinDelta = XMVectorAdd(SphereCenter, BoxExtents);
	XMVECTOR MaxDelta = XMVectorSubtract(SphereCenter, BoxExtents);

	// Choose value for each dimension based on the comparison.
	d = XMVectorSelect(d, MinDelta, LessThanMin);
	d = XMVectorSelect(d, MaxDelta, GreaterThanMax);

	// Use a dot-product to square them and sum them together.
	XMVECTOR d2 = XMVector3Dot(d, d);

	XMFLOAT3 distance;
	XMStoreFloat3(&distance, XMVectorSubtract(d2, XMVectorMultiply(SphereRadius, SphereRadius)));

	if (XMVector3LessOrEqual(d2, XMVectorMultiply(SphereRadius, SphereRadius)))
	{
		return { sqrtf(fabsf(distance.x)), 0.0f, sqrtf(fabsf(distance.z)) };
	}

	return { 0.0f, 0.0f, 0.0f };
}

XMFLOAT3 BoundingVolume::insectCheck(const BoundingSphere& other) const noexcept
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
		const auto distance = (combinedRadius - sphereDistance) * 0.5f;

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
