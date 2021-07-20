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

struct Sphere
{
	XMFLOAT4 CenterRadius;

	Sphere(void) = default;
	Sphere(const XMFLOAT3& center, const float radius) :
		CenterRadius(center.x, center.y, center.z, radius)
	{}
	~Sphere(void) = default;
};

struct AABB
{
	XMFLOAT4 Min;
	XMFLOAT4 Max;

	AABB(void) = default;
	AABB(const XMFLOAT3& _min, const XMFLOAT3& _max) :
		Min(_min.x, _min.y, _min.z, 0.0f),
		Max(_max.x, _max.y, _max.z, 0.0f)
	{}
	~AABB(void) = default;
};

struct OBB
{
	XMFLOAT4 Min;
	XMFLOAT4 Max;

	OBB(void) = default;
	OBB(const XMFLOAT3& _min, const XMFLOAT3& _max) :
		Min(_min.x, _min.y, _min.z, 1.0f),
		Max(_max.x, _max.y, _max.z, 1.0f)
	{}
	~OBB(void) = default;
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

	XMFLOAT3&& insectCheck(const OBB&) noexcept;
	XMFLOAT3&& insectCheck(const AABB&) noexcept;
	XMFLOAT3&& insectCheck(const Sphere&) noexcept;

private:
	OBB m_OBB;
	AABB m_AABB;
	Sphere m_Sphere;

	OBB m_OBBTransformed;
	AABB m_AABBTransformed;
	Sphere m_SphereTransformed;

	Range rangeOBB;
	Range rangeAABB;
	Range rangeSphere;

	static BoundingVolumeTestType TestType;

	friend class Model;

};

