#include "ObjLoader2.h"

using namespace std;

void ObjLoader::Load(
	string filename,
	Vertex*& outVertices,
	int& vcount,
	DWORD*& outIndices,
	int& icount
) {
	ifstream input(filename);

	if (!input.is_open())
	{
		Log::Error("Obj File couldn't be loaded.");
	}

	vector<Vertex> vertices;
	vector<XMFLOAT2> texcoord;
	vector<DWORD> indices;

	string line;
	while (getline(input, line))
	{
		if (!line.empty()) {
			if (!line.compare(0, 2, "v ")) {
				size_t start;
				size_t end = 0;
				vector<string> xyz;

				while ((start = line.find_first_not_of(' ', end)) != string::npos)
				{
					end = line.find(' ', start);
					xyz.push_back(line.substr(start, end - start));
				}
				vertices.push_back({ XMFLOAT3(stof(xyz[1]), stof(xyz[2]), stof(xyz[3])), XMFLOAT4(0.502f, 0.729f, 0.141f, 1.0f), XMFLOAT2(0.0f, 0.0f) });
				vcount++;
			}
			if (!line.compare(0, 2, "vt")) {
				size_t start;
				size_t end = 0;
				vector<string> xy;

				while ((start = line.find_first_not_of(' ', end)) != string::npos)
				{
					end = line.find(' ', start);
					xy.push_back(line.substr(start, end - start));
				}
				texcoord.push_back(XMFLOAT2(stof(xy[1]), 1.0f - stof(xy[2])));
			}
			if (line[0] == 'f') {
				size_t start;
				size_t end = 0;
				vector<string> i;

				while ((start = line.find_first_not_of(" /", end)) != string::npos)
				{
					end = line.find(' ', start);
					if (end > line.find('/', start)) end = line.find('/', start);
					i.push_back(line.substr(start, end - start));
				}
				indices.push_back(stoi(i[1]) - 1);
				vertices[stoi(i[1]) - 1].TexCoord = texcoord[stoi(i[2]) - 1];
				indices.push_back(stoi(i[3]) - 1);
				vertices[stoi(i[3]) - 1].TexCoord = texcoord[stoi(i[4]) - 1];
				indices.push_back(stoi(i[5]) - 1);
				vertices[stoi(i[5]) - 1].TexCoord = texcoord[stoi(i[6]) - 1];
				icount += 3;
			}
		}
	}

	if (!input.eof())
	{
		Log::Error("The end of file wasn't found.");
	}

	input.close();

	outVertices = new Vertex[vcount];
	outIndices = new DWORD[icount];

	copy(&vertices[0], &vertices[0] + vcount, outVertices);
	copy(&indices[0], &indices[0] + icount, outIndices);

}