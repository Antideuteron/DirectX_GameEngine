#pragma once

class Model;

enum class BoundingVolumeTestType
{
	OBB,
	AABB,
	Sphere,
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

public:
	static void SimpleCollisionCheck(const std::vector<BoundingVolume*>& models) noexcept;

	static BoundingVolumeTestType CullingUpdate(void) noexcept;
	static void FrustumCull(const std::vector<Model*>& models, std::vector<Model*>&) noexcept;


	static std::vector<BoundingVolume*> broad(const std::vector<BoundingVolume*>& models) noexcept;
	static bool narrow(const std::vector<BoundingVolume*>& models) noexcept;
	static inline bool SweepNPrune(const std::vector<BoundingVolume*>& models) noexcept { return narrow(broad(models)); }

private:
	
	
	static BoundingVolumeTestType TestType;

	friend class Model;

};

