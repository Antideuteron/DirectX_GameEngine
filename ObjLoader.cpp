#include "ObjLoader.h"

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

static std::vector<V3> vertices;
static std::vector<V2> texCoords;
static std::vector<DWORD> indices;

static void addToMap(std::vector<std::pair<std::string, Vertex>>& map, const std::string& index) noexcept
{
	const auto _indices = split(index, "/");

	int indexVertex = atoi(_indices[0].c_str()) - 1;
	int indexTexCoord = atoi(_indices[1].c_str()) - 1;

	indices.push_back(indexVertex);

	auto it = map.begin();

	while (it != map.end())
	{
		if (it->first == index) break;

		++it;
	}

	if (it != map.end()) return;

	Vertex v;

	v.Position = XMFLOAT3(vertices[indexVertex].x, vertices[indexVertex].y, vertices[indexVertex].z);
	v.TexCoord = XMFLOAT2(texCoords[indexTexCoord].u, texCoords[indexTexCoord].v);
	v.Color = { 1.0f, 1.0f, 1.0f, 1.0f };

	map.push_back({ index, v });
}

void ObjLoader2::Load(
	std::string filename,
	Vertex*& outVertices, int& vcount,
	DWORD*& outIndices, int& icount
)
{
	std::string to;
	std::ifstream t(filename);

	std::vector<std::pair<std::string, Vertex>> vertexMap;

	vertices.resize(0);
	texCoords.resize(0);
	indices.resize(0);

	while (std::getline(t, to))
	{
		const auto comps = split(to, " ");

		if (comps.size() == 0) continue;

		if (comps[0] == "v")
		{
			vertices.emplace_back(strtof(comps[1].c_str(), nullptr), strtof(comps[2].c_str(), nullptr), strtof(comps[3].c_str(), nullptr));
		}
		else if (comps[0] == "vt")
		{
			// TODO y-Achse drehen maybe?
			texCoords.emplace_back(strtof(comps[1].c_str(), nullptr), strtof(comps[2].c_str(), nullptr));
		}
		else if (comps[0] == "f")
		{
			addToMap(vertexMap, comps[1]);
			addToMap(vertexMap, comps[2]);
			addToMap(vertexMap, comps[3]);
		}
	}

	vcount = (int)vertexMap.size();
	outVertices = new Vertex[vcount];

	for (size_t i = 0; i < vertexMap.size(); ++i)
	{
		outVertices[i] = std::move(vertexMap[i].second);
	}

	icount = (int)indices.size();
	outIndices = new DWORD[icount];

	memcpy(outIndices, indices.data(), sizeof(DWORD) * icount);

#if defined(_DEBUG) && 0
	for (int i = 0; i < vcount; i++) {
		Log::Info(std::to_string(outVertices[i].Position.x) + "/" + std::to_string(outVertices[i].Position.y) + "/" + std::to_string(outVertices[i].Position.z) + "  texcoord: " + "/" + std::to_string(outVertices[i].TexCoord.x) + "/" + std::to_string(outVertices[i].TexCoord.y));
	}
	for (int i = 0; i < icount; i++) {
		Log::Info(std::to_string(outIndices[i]));
	}
#endif
}
