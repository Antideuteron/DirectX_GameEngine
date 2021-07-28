#include "BoundingVolume.h"

#include "Model.h"
#include "Camera.hpp"
#include "Keyboard.hpp"

constexpr const float max_float = std::numeric_limits<float>::max();
constexpr const float min_float = std::numeric_limits<float>::min();

BoundingVolumeTestType BoundingVolume::TestType = BoundingVolumeTestType::AABB;

BoundingVolume::BoundingVolume(std::vector<Vertex>& vertices) noexcept
{
	BoundingBox::CreateFromPoints(m_AABB, vertices.size(), (XMFLOAT3*)vertices.data(), sizeof(Vertex));
	BoundingSphere::CreateFromPoints(m_Sphere, vertices.size(), (XMFLOAT3*)vertices.data(), sizeof(Vertex));
	BoundingOrientedBox::CreateFromPoints(m_OBB, vertices.size(), (XMFLOAT3*)vertices.data(), sizeof(Vertex));
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
}

void BoundingVolume::SimpleCollisionCheck(const std::vector<BoundingVolume*>& models) noexcept
{
	auto* player = &Camera::Body();

	for (const auto& model : models)
	{
		XMFLOAT3 resolution;
		if (model->Intersects(player, resolution))
		{
			Camera::m_Position.x -= Camera::Translation().x;
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

static BoundingVolumeTestType s_Type = BoundingVolumeTestType::AABB;

BoundingVolumeTestType BoundingVolume::CullingUpdate(void) noexcept
{
	BoundingVolumeTestType type = s_Type;

	if (Keyboard::IsPressed(sc_1)) type = BoundingVolumeTestType::Sphere;
	else if (Keyboard::IsPressed(sc_2)) type = BoundingVolumeTestType::AABB;
	else if (Keyboard::IsPressed(sc_3)) type = BoundingVolumeTestType::OBB;

	return type;
}

void BoundingVolume::FrustumCull(const std::vector<Model*>& models, std::vector<Model*>& toRender) noexcept
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

std::vector<BoundingVolume*> BoundingVolume::broad(const std::vector<BoundingVolume*>& models) noexcept
{
	std::vector<BoundingVolume*> intersections;
	const auto start = std::chrono::system_clock::now();

	for (const auto model : models) if (model->m_AABBTransformed.Intersects(Camera::Body().m_SphereTransformed)) intersections.push_back(model);

	const auto end = std::chrono::system_clock::now();
	const std::chrono::duration<double> diff = (end - start);

	Log::Info((std::wstringstream() << L"Broad Phase: " << diff.count() * 1000.0 << " ms - " << models.size() << " models tested - " << (intersections.size() != 0 ? "COLLISION" : "NO COLLISION")).str());

	return intersections;
}

bool BoundingVolume::narrow(const std::vector<BoundingVolume*>& models) noexcept
{
	bool result = false;
	const auto start = std::chrono::system_clock::now();

	if (models.size() == 0) return result;

	for (const auto model : models)
	{
		if (model->m_OBBTransformed.Intersects(Camera::Body().m_SphereTransformed))
		{
			result = true;
		}
	}

	const auto end = std::chrono::system_clock::now();
	const std::chrono::duration<double> diff = (end - start);

	Log::Info((std::wstringstream() << L"NarrowPhase: " << diff.count() * 1000.0 << " ms - " << models.size() << " models tested").str());

	return result;
}
