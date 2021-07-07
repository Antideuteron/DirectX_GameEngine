#include "ObjLoader.h"

// TODO Neuerstellen des OBJLoaders

static vector<std::string> split(const std::string& str, const std::string& delim)
{
	vector<std::string> tokens;
	size_t	prev = 0,
					pos = 0;

	do
	{
		pos = str.find(delim, prev);

		if (pos == std::string::npos) pos = str.length();

		std::string token = str.substr(prev, pos - prev);

		if (!token.empty()) tokens.push_back(token);

		prev = pos + delim.length();
	} while (pos < str.length() && prev < str.length());

	return tokens;
}

struct V2
{
	float u;
	float v;

	V2(void) noexcept = default;
	V2(const float u, const float v) : u(u), v(v) {}
};

struct V3
{
	float x;
	float y;
	float z;

	V3(void) noexcept = default;
	V3(const float x, const float y, const float z) : x(x), y(y), z(z) {}
};

static std::vector<V3> positions;
static std::vector<V2> texCoords;
static std::vector<Vertex> vertices;
static std::vector<DWORD> indices;

static void add(const std::string& face)
{
	const auto comps = split(face, "/");

	const int first  = atoi(comps[0].c_str()) - 1;
	const int second = atoi(comps[1].c_str()) - 1;

	const auto& position = positions[first];
	const auto& texcoord = texCoords[second];

	const Vertex vertex = {
		{ position.x, position.y, position.z },
		{ 1.0f, 1.0f, 1.0f, 1.0f },	// color - white/opaque
		{ texcoord.u, texcoord.v }
	};

	vertices.emplace_back(vertex);
	const DWORD index = indices.size();
	indices.push_back(index);
}

void OBJLoader::Load(const std::string& filename, Vertex*& outVertices, int& vcount, DWORD*& outIndices, int& icount) noexcept
{
	std::string to;
	std::ifstream t(filename);

	indices.resize(0);
	vertices.resize(0);
	positions.resize(0);
	texCoords.resize(0);

	while (std::getline(t, to))
	{
		const auto comps = split(to, " ");

		if (comps.size() == 0) continue;

		if (comps[0] == "#") continue;
		if (comps[0] == "o") continue;
		if (comps[0] == "s") continue;

		if (comps[0] == "v")
		{
			positions.emplace_back(strtof(comps[1].c_str(), nullptr), strtof(comps[2].c_str(), nullptr), strtof(comps[3].c_str(), nullptr));
		}
		else if (comps[0] == "vt")
		{
			texCoords.emplace_back(strtof(comps[1].c_str(), nullptr), strtof(comps[2].c_str(), nullptr));
		}
		else if (comps[0] == "f")
		{
			add(comps[1]);
			add(comps[2]);
			add(comps[3]);
		}
	}

	vcount = (int)vertices.size();
	outVertices = new Vertex[vcount];

	memcpy(outVertices, vertices.data(), vcount * sizeof(Vertex));

	icount = (int)indices.size();
	outIndices = new DWORD[icount];

	memcpy(outIndices, indices.data(), sizeof(DWORD)* icount);
}
