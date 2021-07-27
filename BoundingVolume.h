#pragma once

struct Range
{
	float xbegin;
	float xend;
	float ybegin;
	float yend;
	float zbegin;
	float zend;

	Range(void) noexcept = default;
	~Range(void) noexcept = default;
};

enum class BoundingVolumeTestType
{
	OBB,
	AABB,
	Sphere
};

class BoundingVolume
{
public:
	BoundingVolume(void) noexcept = default;
	BoundingVolume(std::vector<Vertex>& vertices) noexcept;
	~BoundingVolume(void) noexcept = default;

	bool Intersects(BoundingVolume* other, XMFLOAT3& resolution) noexcept;
	void Update(XMFLOAT3* position, XMFLOAT4* rotation) noexcept;

	static inline BoundingVolumeTestType& BVTT(void) noexcept { return TestType; }

	XMFLOAT3 insectCheck(const BoundingBox&) const noexcept;
	XMFLOAT3 insectCheck(const BoundingSphere&) const noexcept;
	XMFLOAT3 insectCheck(const BoundingOrientedBox&) const noexcept;
	XMFLOAT3 insectCheckAABBSphere(const BoundingSphere& sphere) const noexcept;
	XMFLOAT3 insectCheckOBBSphere(const BoundingSphere& sphere) const noexcept;

	BoundingBox m_AABB;
	BoundingSphere m_Sphere;
	BoundingOrientedBox m_OBB;
	BoundingBox m_AABBTransformed;
	BoundingSphere m_SphereTransformed;
	BoundingOrientedBox m_OBBTransformed;
	Range rangeAABB;

public:
	static void SimpleCollisionCheck(const std::vector<BoundingVolume*>& models) noexcept;

private:
	
	
	static BoundingVolumeTestType TestType;

	friend class Model;

};

