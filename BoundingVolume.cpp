#include "BoundingVolume.h"

#include "Model.h"
#include "Camera.hpp"
#include "Keyboard.hpp"

constexpr const float max_float = std::numeric_limits<float>::max();
constexpr const float min_float = std::numeric_limits<float>::min();

static BoundingVolumeTestType s_Type = BoundingVolumeTestType::AABB;
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
		case BoundingVolumeTestType::Sphere: return insectCheck(other->m_SphereTransformed, resolution);
		case BoundingVolumeTestType::AABB:   return insectCheck(other->m_AABBTransformed, resolution);
		case BoundingVolumeTestType::OBB:    return insectCheck(other->m_OBBTransformed, resolution);
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

bool BoundingVolume::SimpleCollisionCheck(const std::vector<BoundingVolume*>& models) noexcept
{
	auto* player = &Camera::Body();

  XMFLOAT3 resolution;

	for (const auto& model : models) return model->Intersects(player, resolution);
}

bool BoundingVolume::insectCheck(const BoundingOrientedBox& other, XMFLOAT3& resolution) const noexcept
{
	// Build the 3x3 rotation matrix that defines the orientation of B relative to A.
	XMVECTOR A_quat = XMLoadFloat4(&m_OBBTransformed.Orientation);
	XMVECTOR B_quat = XMLoadFloat4(&other.Orientation);

	assert(DirectX::Internal::XMQuaternionIsUnit(A_quat));
	assert(DirectX::Internal::XMQuaternionIsUnit(B_quat));

	XMVECTOR Q = XMQuaternionMultiply(A_quat, XMQuaternionConjugate(B_quat));
	XMMATRIX R = XMMatrixRotationQuaternion(Q);

	// Compute the translation of B relative to A.
	XMVECTOR A_cent = XMLoadFloat3(&m_OBBTransformed.Center);
	XMVECTOR B_cent = XMLoadFloat3(&other.Center);
	XMVECTOR t = XMVector3InverseRotate(XMVectorSubtract(B_cent, A_cent), A_quat);

	//
	// h(A) = extents of A.
	// h(B) = extents of B.
	//
	// a(u) = axes of A = (1,0,0), (0,1,0), (0,0,1)
	// b(u) = axes of B relative to A = (r00,r10,r20), (r01,r11,r21), (r02,r12,r22)
	//
	// For each possible separating axis l:
	//   d(A) = sum (for i = u,v,w) h(A)(i) * abs( a(i) dot l )
	//   d(B) = sum (for i = u,v,w) h(B)(i) * abs( b(i) dot l )
	//   if abs( t dot l ) > d(A) + d(B) then disjoint
	//

	// Load extents of A and B.
	XMVECTOR h_A = XMLoadFloat3(&m_OBBTransformed.Extents);
	XMVECTOR h_B = XMLoadFloat3(&other.Extents);

	// Rows. Note R[0,1,2]X.w = 0.
	XMVECTOR R0X = R.r[0];
	XMVECTOR R1X = R.r[1];
	XMVECTOR R2X = R.r[2];

	R = XMMatrixTranspose(R);

	// Columns. Note RX[0,1,2].w = 0.
	XMVECTOR RX0 = R.r[0];
	XMVECTOR RX1 = R.r[1];
	XMVECTOR RX2 = R.r[2];

	// Absolute value of rows.
	XMVECTOR AR0X = XMVectorAbs(R0X);
	XMVECTOR AR1X = XMVectorAbs(R1X);
	XMVECTOR AR2X = XMVectorAbs(R2X);

	// Absolute value of columns.
	XMVECTOR ARX0 = XMVectorAbs(RX0);
	XMVECTOR ARX1 = XMVectorAbs(RX1);
	XMVECTOR ARX2 = XMVectorAbs(RX2);

	// Test each of the 15 possible seperating axii.
	XMVECTOR d, d_A, d_B;

	// l = a(u) = (1, 0, 0)
	// t dot l = t.x
	// d(A) = h(A).x
	// d(B) = h(B) dot abs(r00, r01, r02)
	d = XMVectorSplatX(t);
	d_A = XMVectorSplatX(h_A);
	d_B = XMVector3Dot(h_B, AR0X);
	XMVECTOR NoIntersection = XMVectorGreater(XMVectorAbs(d), XMVectorAdd(d_A, d_B));

	// l = a(v) = (0, 1, 0)
	// t dot l = t.y
	// d(A) = h(A).y
	// d(B) = h(B) dot abs(r10, r11, r12)
	d = XMVectorSplatY(t);
	d_A = XMVectorSplatY(h_A);
	d_B = XMVector3Dot(h_B, AR1X);
	NoIntersection = XMVectorOrInt(NoIntersection, XMVectorGreater(XMVectorAbs(d), XMVectorAdd(d_A, d_B)));

	// l = a(w) = (0, 0, 1)
	// t dot l = t.z
	// d(A) = h(A).z
	// d(B) = h(B) dot abs(r20, r21, r22)
	d = XMVectorSplatZ(t);
	d_A = XMVectorSplatZ(h_A);
	d_B = XMVector3Dot(h_B, AR2X);
	NoIntersection = XMVectorOrInt(NoIntersection, XMVectorGreater(XMVectorAbs(d), XMVectorAdd(d_A, d_B)));

	// l = b(u) = (r00, r10, r20)
	// d(A) = h(A) dot abs(r00, r10, r20)
	// d(B) = h(B).x
	d = XMVector3Dot(t, RX0);
	d_A = XMVector3Dot(h_A, ARX0);
	d_B = XMVectorSplatX(h_B);
	NoIntersection = XMVectorOrInt(NoIntersection, XMVectorGreater(XMVectorAbs(d), XMVectorAdd(d_A, d_B)));

	// l = b(v) = (r01, r11, r21)
	// d(A) = h(A) dot abs(r01, r11, r21)
	// d(B) = h(B).y
	d = XMVector3Dot(t, RX1);
	d_A = XMVector3Dot(h_A, ARX1);
	d_B = XMVectorSplatY(h_B);
	NoIntersection = XMVectorOrInt(NoIntersection, XMVectorGreater(XMVectorAbs(d), XMVectorAdd(d_A, d_B)));

	// l = b(w) = (r02, r12, r22)
	// d(A) = h(A) dot abs(r02, r12, r22)
	// d(B) = h(B).z
	d = XMVector3Dot(t, RX2);
	d_A = XMVector3Dot(h_A, ARX2);
	d_B = XMVectorSplatZ(h_B);
	NoIntersection = XMVectorOrInt(NoIntersection, XMVectorGreater(XMVectorAbs(d), XMVectorAdd(d_A, d_B)));

	// l = a(u) x b(u) = (0, -r20, r10)
	// d(A) = h(A) dot abs(0, r20, r10)
	// d(B) = h(B) dot abs(0, r02, r01)
	d = XMVector3Dot(t, XMVectorPermute<XM_PERMUTE_0W, XM_PERMUTE_1Z, XM_PERMUTE_0Y, XM_PERMUTE_0X>(RX0, XMVectorNegate(RX0)));
	d_A = XMVector3Dot(h_A, XMVectorSwizzle<XM_SWIZZLE_W, XM_SWIZZLE_Z, XM_SWIZZLE_Y, XM_SWIZZLE_X>(ARX0));
	d_B = XMVector3Dot(h_B, XMVectorSwizzle<XM_SWIZZLE_W, XM_SWIZZLE_Z, XM_SWIZZLE_Y, XM_SWIZZLE_X>(AR0X));
	NoIntersection = XMVectorOrInt(NoIntersection, XMVectorGreater(XMVectorAbs(d), XMVectorAdd(d_A, d_B)));

	// l = a(u) x b(v) = (0, -r21, r11)
	// d(A) = h(A) dot abs(0, r21, r11)
	// d(B) = h(B) dot abs(r02, 0, r00)
	d = XMVector3Dot(t, XMVectorPermute<XM_PERMUTE_0W, XM_PERMUTE_1Z, XM_PERMUTE_0Y, XM_PERMUTE_0X>(RX1, XMVectorNegate(RX1)));
	d_A = XMVector3Dot(h_A, XMVectorSwizzle<XM_SWIZZLE_W, XM_SWIZZLE_Z, XM_SWIZZLE_Y, XM_SWIZZLE_X>(ARX1));
	d_B = XMVector3Dot(h_B, XMVectorSwizzle<XM_SWIZZLE_Z, XM_SWIZZLE_W, XM_SWIZZLE_X, XM_SWIZZLE_Y>(AR0X));
	NoIntersection = XMVectorOrInt(NoIntersection, XMVectorGreater(XMVectorAbs(d), XMVectorAdd(d_A, d_B)));

	// l = a(u) x b(w) = (0, -r22, r12)
	// d(A) = h(A) dot abs(0, r22, r12)
	// d(B) = h(B) dot abs(r01, r00, 0)
	d = XMVector3Dot(t, XMVectorPermute<XM_PERMUTE_0W, XM_PERMUTE_1Z, XM_PERMUTE_0Y, XM_PERMUTE_0X>(RX2, XMVectorNegate(RX2)));
	d_A = XMVector3Dot(h_A, XMVectorSwizzle<XM_SWIZZLE_W, XM_SWIZZLE_Z, XM_SWIZZLE_Y, XM_SWIZZLE_X>(ARX2));
	d_B = XMVector3Dot(h_B, XMVectorSwizzle<XM_SWIZZLE_Y, XM_SWIZZLE_X, XM_SWIZZLE_W, XM_SWIZZLE_Z>(AR0X));
	NoIntersection = XMVectorOrInt(NoIntersection, XMVectorGreater(XMVectorAbs(d), XMVectorAdd(d_A, d_B)));

	// l = a(v) x b(u) = (r20, 0, -r00)
	// d(A) = h(A) dot abs(r20, 0, r00)
	// d(B) = h(B) dot abs(0, r12, r11)
	d = XMVector3Dot(t, XMVectorPermute<XM_PERMUTE_0Z, XM_PERMUTE_0W, XM_PERMUTE_1X, XM_PERMUTE_0Y>(RX0, XMVectorNegate(RX0)));
	d_A = XMVector3Dot(h_A, XMVectorSwizzle<XM_SWIZZLE_Z, XM_SWIZZLE_W, XM_SWIZZLE_X, XM_SWIZZLE_Y>(ARX0));
	d_B = XMVector3Dot(h_B, XMVectorSwizzle<XM_SWIZZLE_W, XM_SWIZZLE_Z, XM_SWIZZLE_Y, XM_SWIZZLE_X>(AR1X));
	NoIntersection = XMVectorOrInt(NoIntersection, XMVectorGreater(XMVectorAbs(d), XMVectorAdd(d_A, d_B)));

	// l = a(v) x b(v) = (r21, 0, -r01)
	// d(A) = h(A) dot abs(r21, 0, r01)
	// d(B) = h(B) dot abs(r12, 0, r10)
	d = XMVector3Dot(t, XMVectorPermute<XM_PERMUTE_0Z, XM_PERMUTE_0W, XM_PERMUTE_1X, XM_PERMUTE_0Y>(RX1, XMVectorNegate(RX1)));
	d_A = XMVector3Dot(h_A, XMVectorSwizzle<XM_SWIZZLE_Z, XM_SWIZZLE_W, XM_SWIZZLE_X, XM_SWIZZLE_Y>(ARX1));
	d_B = XMVector3Dot(h_B, XMVectorSwizzle<XM_SWIZZLE_Z, XM_SWIZZLE_W, XM_SWIZZLE_X, XM_SWIZZLE_Y>(AR1X));
	NoIntersection = XMVectorOrInt(NoIntersection, XMVectorGreater(XMVectorAbs(d), XMVectorAdd(d_A, d_B)));

	// l = a(v) x b(w) = (r22, 0, -r02)
	// d(A) = h(A) dot abs(r22, 0, r02)
	// d(B) = h(B) dot abs(r11, r10, 0)
	d = XMVector3Dot(t, XMVectorPermute<XM_PERMUTE_0Z, XM_PERMUTE_0W, XM_PERMUTE_1X, XM_PERMUTE_0Y>(RX2, XMVectorNegate(RX2)));
	d_A = XMVector3Dot(h_A, XMVectorSwizzle<XM_SWIZZLE_Z, XM_SWIZZLE_W, XM_SWIZZLE_X, XM_SWIZZLE_Y>(ARX2));
	d_B = XMVector3Dot(h_B, XMVectorSwizzle<XM_SWIZZLE_Y, XM_SWIZZLE_X, XM_SWIZZLE_W, XM_SWIZZLE_Z>(AR1X));
	NoIntersection = XMVectorOrInt(NoIntersection, XMVectorGreater(XMVectorAbs(d), XMVectorAdd(d_A, d_B)));

	// l = a(w) x b(u) = (-r10, r00, 0)
	// d(A) = h(A) dot abs(r10, r00, 0)
	// d(B) = h(B) dot abs(0, r22, r21)
	d = XMVector3Dot(t, XMVectorPermute<XM_PERMUTE_1Y, XM_PERMUTE_0X, XM_PERMUTE_0W, XM_PERMUTE_0Z>(RX0, XMVectorNegate(RX0)));
	d_A = XMVector3Dot(h_A, XMVectorSwizzle<XM_SWIZZLE_Y, XM_SWIZZLE_X, XM_SWIZZLE_W, XM_SWIZZLE_Z>(ARX0));
	d_B = XMVector3Dot(h_B, XMVectorSwizzle<XM_SWIZZLE_W, XM_SWIZZLE_Z, XM_SWIZZLE_Y, XM_SWIZZLE_X>(AR2X));
	NoIntersection = XMVectorOrInt(NoIntersection, XMVectorGreater(XMVectorAbs(d), XMVectorAdd(d_A, d_B)));

	// l = a(w) x b(v) = (-r11, r01, 0)
	// d(A) = h(A) dot abs(r11, r01, 0)
	// d(B) = h(B) dot abs(r22, 0, r20)
	d = XMVector3Dot(t, XMVectorPermute<XM_PERMUTE_1Y, XM_PERMUTE_0X, XM_PERMUTE_0W, XM_PERMUTE_0Z>(RX1, XMVectorNegate(RX1)));
	d_A = XMVector3Dot(h_A, XMVectorSwizzle<XM_SWIZZLE_Y, XM_SWIZZLE_X, XM_SWIZZLE_W, XM_SWIZZLE_Z>(ARX1));
	d_B = XMVector3Dot(h_B, XMVectorSwizzle<XM_SWIZZLE_Z, XM_SWIZZLE_W, XM_SWIZZLE_X, XM_SWIZZLE_Y>(AR2X));
	NoIntersection = XMVectorOrInt(NoIntersection, XMVectorGreater(XMVectorAbs(d), XMVectorAdd(d_A, d_B)));

	// l = a(w) x b(w) = (-r12, r02, 0)
	// d(A) = h(A) dot abs(r12, r02, 0)
	// d(B) = h(B) dot abs(r21, r20, 0)
	d = XMVector3Dot(t, XMVectorPermute<XM_PERMUTE_1Y, XM_PERMUTE_0X, XM_PERMUTE_0W, XM_PERMUTE_0Z>(RX2, XMVectorNegate(RX2)));
	d_A = XMVector3Dot(h_A, XMVectorSwizzle<XM_SWIZZLE_Y, XM_SWIZZLE_X, XM_SWIZZLE_W, XM_SWIZZLE_Z>(ARX2));
	d_B = XMVector3Dot(h_B, XMVectorSwizzle<XM_SWIZZLE_Y, XM_SWIZZLE_X, XM_SWIZZLE_W, XM_SWIZZLE_Z>(AR2X));
	NoIntersection = XMVectorOrInt(NoIntersection, XMVectorGreater(XMVectorAbs(d), XMVectorAdd(d_A, d_B)));

	// No seperating axis found, boxes must intersect.
	if (XMVector4NotEqualInt(NoIntersection, XMVectorTrueInt()))
	{
		resolution = { 1.0f, 1.0f, 1.0f };

		return true;
	}

	return false;
}

bool BoundingVolume::insectCheck(const BoundingBox& other, XMFLOAT3& resolution) const noexcept
{
	XMVECTOR CenterA = XMLoadFloat3(&m_AABBTransformed.Center);
	XMVECTOR ExtentsA = XMLoadFloat3(&m_AABBTransformed.Extents);

	XMVECTOR CenterB = XMLoadFloat3(&other.Center);
	XMVECTOR ExtentsB = XMLoadFloat3(&other.Extents);

	XMVECTOR MinA = XMVectorSubtract(CenterA, ExtentsA);
	XMVECTOR MaxA = XMVectorAdd(CenterA, ExtentsA);

	XMVECTOR MinB = XMVectorSubtract(CenterB, ExtentsB);
	XMVECTOR MaxB = XMVectorAdd(CenterB, ExtentsB);

	// for each i in (x, y, z) if a_min(i) > b_max(i) or b_min(i) > a_max(i) then return false
	XMVECTOR Disjoint = XMVectorOrInt(XMVectorGreater(MinA, MaxB), XMVectorGreater(MinB, MaxA));

	if (!DirectX::Internal::XMVector3AnyTrue(Disjoint))
	{
		resolution = { 1.0f, 0.0f, 1.0f };

		return true;
	}

	return false;
}

bool BoundingVolume::insectCheckAABBSphere(const BoundingSphere& sphere, XMFLOAT3& resolution) const noexcept // WTF
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
		resolution = { 1.0f, 0.0f, 1.0f };

		return true;
	}

	return false;
}

bool BoundingVolume::insectCheckOBBSphere(const BoundingSphere& sphere, XMFLOAT3& resolution) const noexcept
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
		resolution = { 1.0f, 0.0f, 1.0f };

		return true;
	}

	return false;
}

bool BoundingVolume::insectCheck(const BoundingSphere& other, XMFLOAT3& resolution) const noexcept
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

		resolution = { distance, 0.0f, distance };

		return true;
	}

	// no intersection
	return false;
}

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
	XMFLOAT3 f;

	const auto BVTT = BoundingVolume::BVTT();
	BoundingVolume::BVTT() = BoundingVolumeTestType::AABB;
	for (const auto model : models) if (model->Intersects(&Camera::Body(), f)) intersections.push_back(model);
	BoundingVolume::BVTT() = BVTT;

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
		XMFLOAT3 f;

		const auto BVTT = BoundingVolume::BVTT();
		BoundingVolume::BVTT() = BoundingVolumeTestType::OBB;
		if (model->Intersects(&Camera::Body(), f))
		{
			result = true;
		}
		BoundingVolume::BVTT() = BVTT;
	}

	const auto end = std::chrono::system_clock::now();
	const std::chrono::duration<double> diff = (end - start);

	Log::Info((std::wstringstream() << L"NarrowPhase: " << diff.count() * 1000.0 << " ms - " << models.size() << " models tested").str());

	return result;
}
