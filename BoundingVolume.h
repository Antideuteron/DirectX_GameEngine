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

	bool insectCheck(const BoundingBox&, XMFLOAT3&) const noexcept;
	bool insectCheck(const BoundingSphere&, XMFLOAT3&) const noexcept;
	bool insectCheck(const BoundingOrientedBox&, XMFLOAT3&) const noexcept;
	bool insectCheckAABBSphere(const BoundingSphere& sphere, XMFLOAT3&) const noexcept;
	bool insectCheckOBBSphere(const BoundingSphere& sphere, XMFLOAT3&) const noexcept;

	BoundingBox m_AABB;
	BoundingSphere m_Sphere;
	BoundingOrientedBox m_OBB;
	BoundingBox m_AABBTransformed;
	BoundingSphere m_SphereTransformed;
	BoundingOrientedBox m_OBBTransformed;

public:
	static bool SimpleCollisionCheck(const std::vector<BoundingVolume*>& models) noexcept;

	static void FrustumCull(const std::vector<Model*>& models, std::vector<Model*>&) noexcept;


	static std::vector<BoundingVolume*> broad(const std::vector<BoundingVolume*>& models) noexcept;
	static bool narrow(const std::vector<BoundingVolume*>& models) noexcept;
	static inline bool SweepNPrune(const std::vector<BoundingVolume*>& models) noexcept { return narrow(broad(models)); }

private:
	static BoundingVolumeTestType CullingUpdate(void) noexcept;


	static BoundingVolumeTestType TestType;

	friend class Model;

};

