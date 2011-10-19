//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include <glf/iomodel.hpp>
#include <glf/ioimage.hpp>
#include <glf/utils.hpp>
//------------------------------------------------------------------------------
#include <cstring>
#include <cstdio>
#include <cmath>
#include <cstdlib>
#include <cfloat>
#include <climits>
#include <ctype.h>
#include <algorithm>
#include <limits>
#include <vector>
#include <map>
#include <fstream>
#include <cassert>


//------------------------------------------------------------------------------
// Macros
//------------------------------------------------------------------------------
#define cJSON_False 0
#define cJSON_True 1
#define cJSON_NULL 2
#define cJSON_Number 3
#define cJSON_String 4
#define cJSON_Array 5
#define cJSON_Object 6
#define cJSON_IsReference 256
//------------------------------------------------------------------------------
#define ENABLE_ANISOSTROPIC_FILTERING	1
#define MAX_ANISOSTROPY					16.f
#define ADD_TBN_HELPERS					0

// OBJ loader
namespace
{
	//--------------------------------------------------------------------------
	// Copyright (c) 2007 dhpoware. All Rights Reserved.
	//
	// Permission is hereby granted, free of charge, to any person obtaining a
	// copy of this software and associated documentation files (the "Software"),
	// to deal in the Software without restriction, including without limitation
	// the rights to use, copy, modify, merge, publish, distribute, sublicense,
	// and/or sell copies of the Software, and to permit persons to whom the
	// Software is furnished to do so, subject to the following conditions:
	//
	// The above copyright notice and this permission notice shall be included in
	// all copies or substantial portions of the Software.
	//
	// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
	// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
	// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
	// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
	// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
	// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
	// IN THE SOFTWARE.
	//--------------------------------------------------------------------------
	//
	// The methods normalize() and scale() are based on source code from
	// http://www.mvps.org/directx/articles/scalemesh9.htm.
	//
	// The addVertex() method is based on source code from the Direct3D MeshFromOBJ
	// sample found in the DirectX SDK.
	//
	// The generateTangents() method is based on public source code from
	// http://www.terathon.com/code/tangent.php.
	//
	// The importGeometryFirstPass(), importGeometrySecondPass(), and
	// importMaterials() methods are based on source code from Nate Robins' OpenGL
	// Tutors programs (http://www.xmission.com/~nate/tutors.html).
	//
	//--------------------------------------------------------------------------
	class ModelOBJ
	{
	public:
		struct Material
		{
			float ambient[4];
			float diffuse[4];
			float specular[4];
			float shininess;        // [0 = min shininess, 1 = max shininess]
			float alpha;            // [0 = fully transparent, 1 = fully opaque]

			std::string name;
			std::string colorMapFilename;
			std::string bumpMapFilename;
		};

		struct Vertex
		{
			float position[3];
			float texCoord[2];
			float normal[3];
			float tangent[4];
			float bitangent[3];
		};

		struct Mesh
		{
			int startIndex;
			int triangleCount;
			const Material *pMaterial;
		};

		ModelOBJ();
		~ModelOBJ();

		void destroy();
		bool import(const char *pszFilename, bool rebuildNormals = false);
		void normalize(float scaleTo = 1.0f, bool center = true);
		void reverseWinding();

		void getCenter(float &x, float &y, float &z) const;
		float getWidth() const;
		float getHeight() const;
		float getLength() const;
		float getRadius() const;

		const int *getIndexBuffer() const;
		int getIndexSize() const;

		const Material &getMaterial(int i) const;
		const Mesh &getMesh(int i) const;

		int getNumberOfIndices() const;
		int getNumberOfMaterials() const;
		int getNumberOfMeshes() const;
		int getNumberOfTriangles() const;
		int getNumberOfVertices() const;

		const std::string &getPath() const;

		const Vertex &getVertex(int i) const;
		const Vertex *getVertexBuffer() const;
		int getVertexSize() const;

		bool hasNormals() const;
		bool hasPositions() const;
		bool hasTangents() const;
		bool hasTextureCoords() const;

	private:
		void addTrianglePos(int index, int material,
			int v0, int v1, int v2);
		void addTrianglePosNormal(int index, int material,
			int v0, int v1, int v2,
			int vn0, int vn1, int vn2);
		void addTrianglePosTexCoord(int index, int material,
			int v0, int v1, int v2,
			int vt0, int vt1, int vt2);
		void addTrianglePosTexCoordNormal(int index, int material,
			int v0, int v1, int v2,
			int vt0, int vt1, int vt2,
			int vn0, int vn1, int vn2);
		int addVertex(int hash, const Vertex *pVertex);
		void bounds(float center[3], float &width, float &height,
			float &length, float &radius) const;
		void buildMeshes();
		void generateNormals();
		void generateTangents();
		void importGeometryFirstPass(FILE *pFile);
		void importGeometrySecondPass(FILE *pFile);
		bool importMaterials(const char *pszFilename);
		void scale(float scaleFactor, float offset[3]);

		bool m_hasPositions;
		bool m_hasTextureCoords;
		bool m_hasNormals;
		bool m_hasTangents;

		int m_numberOfVertexCoords;
		int m_numberOfTextureCoords;
		int m_numberOfNormals;
		int m_numberOfTriangles;
		int m_numberOfMaterials;
		int m_numberOfMeshes;

		float m_center[3];
		float m_width;
		float m_height;
		float m_length;
		float m_radius;

		std::string m_directoryPath;

		std::vector<Mesh> m_meshes;
		std::vector<Material> m_materials;
		std::vector<Vertex> m_vertexBuffer;
		std::vector<int> m_indexBuffer;
		std::vector<int> m_attributeBuffer;
		std::vector<float> m_vertexCoords;
		std::vector<float> m_textureCoords;
		std::vector<float> m_normals;

		std::map<std::string, int> m_materialCache;
		std::map<int, std::vector<int> > m_vertexCache;
	};
	//--------------------------------------------------------------------------
	bool MeshCompFunc(const ModelOBJ::Mesh &lhs, const ModelOBJ::Mesh &rhs)
	{
		return lhs.pMaterial->alpha > rhs.pMaterial->alpha;
	}
	//--------------------------------------------------------------------------
	inline void ModelOBJ::getCenter(float &x, float &y, float &z) const
	{ x = m_center[0]; y = m_center[1]; z = m_center[2]; }
	//--------------------------------------------------------------------------
	inline float ModelOBJ::getWidth() const
	{ return m_width; }
	//--------------------------------------------------------------------------
	inline float ModelOBJ::getHeight() const
	{ return m_height; }
	//--------------------------------------------------------------------------
	inline float ModelOBJ::getLength() const
	{ return m_length; }
	//--------------------------------------------------------------------------
	inline float ModelOBJ::getRadius() const
	{ return m_radius; }
	//--------------------------------------------------------------------------
	inline const int *ModelOBJ::getIndexBuffer() const
	{ return &m_indexBuffer[0]; }
	//--------------------------------------------------------------------------
	inline int ModelOBJ::getIndexSize() const
	{ return static_cast<int>(sizeof(int)); }
	//--------------------------------------------------------------------------
	inline const ModelOBJ::Material &ModelOBJ::getMaterial(int i) const
	{ return m_materials[i]; }
	//--------------------------------------------------------------------------
	inline const ModelOBJ::Mesh &ModelOBJ::getMesh(int i) const
	{ return m_meshes[i]; }
	//--------------------------------------------------------------------------
	inline int ModelOBJ::getNumberOfIndices() const
	{ return m_numberOfTriangles * 3; }
	//--------------------------------------------------------------------------
	inline int ModelOBJ::getNumberOfMaterials() const
	{ return m_numberOfMaterials; }
	//--------------------------------------------------------------------------
	inline int ModelOBJ::getNumberOfMeshes() const
	{ return m_numberOfMeshes; }
	//--------------------------------------------------------------------------
	inline int ModelOBJ::getNumberOfTriangles() const
	{ return m_numberOfTriangles; }
	//--------------------------------------------------------------------------
	inline int ModelOBJ::getNumberOfVertices() const
	{ return static_cast<int>(m_vertexBuffer.size()); }
	//--------------------------------------------------------------------------
	inline const std::string &ModelOBJ::getPath() const
	{ return m_directoryPath; }
	//--------------------------------------------------------------------------
	inline const ModelOBJ::Vertex &ModelOBJ::getVertex(int i) const
	{ return m_vertexBuffer[i]; }
	//--------------------------------------------------------------------------
	inline const ModelOBJ::Vertex *ModelOBJ::getVertexBuffer() const
	{ return &m_vertexBuffer[0]; }
	//--------------------------------------------------------------------------
	inline int ModelOBJ::getVertexSize() const
	{ return static_cast<int>(sizeof(Vertex)); }
	//--------------------------------------------------------------------------
	inline bool ModelOBJ::hasNormals() const
	{ return m_hasNormals; }
	//--------------------------------------------------------------------------
	inline bool ModelOBJ::hasPositions() const
	{ return m_hasPositions; }
	//--------------------------------------------------------------------------
	inline bool ModelOBJ::hasTangents() const
	{ return m_hasTangents; }
	//--------------------------------------------------------------------------
	inline bool ModelOBJ::hasTextureCoords() const
	{ return m_hasTextureCoords; }
	//--------------------------------------------------------------------------
	ModelOBJ::ModelOBJ()
	{
		m_hasPositions = false;
		m_hasNormals = false;
		m_hasTextureCoords = false;
		m_hasTangents = false;

		m_numberOfVertexCoords = 0;
		m_numberOfTextureCoords = 0;
		m_numberOfNormals = 0;
		m_numberOfTriangles = 0;
		m_numberOfMaterials = 0;
		m_numberOfMeshes = 0;

		m_center[0] = m_center[1] = m_center[2] = 0.0f;
		m_width = m_height = m_length = m_radius = 0.0f;
	}
	//--------------------------------------------------------------------------
	ModelOBJ::~ModelOBJ()
	{
		destroy();
	}
	//--------------------------------------------------------------------------
	void ModelOBJ::bounds(float center[3], float &width, float &height,
		                  float &length, float &radius) const
	{
		float xMax = std::numeric_limits<float>::min();
		float yMax = std::numeric_limits<float>::min();
		float zMax = std::numeric_limits<float>::min();

		float xMin = std::numeric_limits<float>::max();
		float yMin = std::numeric_limits<float>::max();
		float zMin = std::numeric_limits<float>::max();

		float x = 0.0f;
		float y = 0.0f;
		float z = 0.0f;

		int numVerts = static_cast<int>(m_vertexBuffer.size());

		for (int i = 0; i < numVerts; ++i)
		{
		    x = m_vertexBuffer[i].position[0];
		    y = m_vertexBuffer[i].position[1];
		    z = m_vertexBuffer[i].position[2];

		    if (x < xMin)
		        xMin = x;

		    if (x > xMax)
		        xMax = x;

		    if (y < yMin)
		        yMin = y;

		    if (y > yMax)
		        yMax = y;

		    if (z < zMin)
		        zMin = z;

		    if (z > zMax)
		        zMax = z;
		}

		center[0] = (xMin + xMax) / 2.0f;
		center[1] = (yMin + yMax) / 2.0f;
		center[2] = (zMin + zMax) / 2.0f;

		width = xMax - xMin;
		height = yMax - yMin;
		length = zMax - zMin;

		radius = std::max(std::max(width, height), length);
	}
	//--------------------------------------------------------------------------
	void ModelOBJ::destroy()
	{
		m_hasPositions = false;
		m_hasTextureCoords = false;
		m_hasNormals = false;
		m_hasTangents = false;

		m_numberOfVertexCoords = 0;
		m_numberOfTextureCoords = 0;
		m_numberOfNormals = 0;
		m_numberOfTriangles = 0;
		m_numberOfMaterials = 0;
		m_numberOfMeshes = 0;

		m_center[0] = m_center[1] = m_center[2] = 0.0f;
		m_width = m_height = m_length = m_radius = 0.0f;

		m_directoryPath.clear();

		m_meshes.clear();
		m_materials.clear();
		m_vertexBuffer.clear();
		m_indexBuffer.clear();
		m_attributeBuffer.clear();

		m_vertexCoords.clear();
		m_textureCoords.clear();
		m_normals.clear();

		m_materialCache.clear();
		m_vertexCache.clear();
	}
	//--------------------------------------------------------------------------
	bool ModelOBJ::import(const char *pszFilename, bool rebuildNormals)
	{
		FILE *pFile = fopen(pszFilename, "r");

		if (!pFile)
		    return false;

		// Extract the directory the OBJ file is in from the file name.
		// This directory path will be used to load the OBJ's associated MTL file.

		m_directoryPath.clear();

		std::string filename = pszFilename;
		std::string::size_type offset = filename.find_last_of('\\');

		if (offset != std::string::npos)
		{
		    m_directoryPath = filename.substr(0, ++offset);
		}
		else
		{
		    offset = filename.find_last_of('/');

		    if (offset != std::string::npos)
		        m_directoryPath = filename.substr(0, ++offset);
		}

		// Import the OBJ file.

		importGeometryFirstPass(pFile);
		rewind(pFile);
		importGeometrySecondPass(pFile);
		fclose(pFile);

		// Perform post import tasks.

		buildMeshes();
		bounds(m_center, m_width, m_height, m_length, m_radius);

		// Build vertex normals if required.

		if (rebuildNormals)
		{
		    generateNormals();
		}
		else
		{
		    if (!hasNormals())
		        generateNormals();
		}

		// Build tangents is required.

		for (int i = 0; i < m_numberOfMaterials; ++i)
		{
		    if (!m_materials[i].bumpMapFilename.empty())
		    {
		        generateTangents();
		        break;
		    }
		}

		return true;
	}
	//--------------------------------------------------------------------------
	void ModelOBJ::normalize(float scaleTo, bool center)
	{
		float width = 0.0f;
		float height = 0.0f;
		float length = 0.0f;
		float radius = 0.0f;
		float centerPos[3] = {0.0f};

		bounds(centerPos, width, height, length, radius);

		float scalingFactor = scaleTo / radius;
		float offset[3] = {0.0f};

		if (center)
		{
		    offset[0] = -centerPos[0];
		    offset[1] = -centerPos[1];
		    offset[2] = -centerPos[2];
		}
		else
		{
		    offset[0] = 0.0f;
		    offset[1] = 0.0f;
		    offset[2] = 0.0f;
		}

		scale(scalingFactor, offset);
		bounds(m_center, m_width, m_height, m_length, m_radius);
	}
	//--------------------------------------------------------------------------
	void ModelOBJ::reverseWinding()
	{
		int swap = 0;

		// Reverse face winding.
		for (int i = 0; i < static_cast<int>(m_indexBuffer.size()); i += 3)
		{
		    swap = m_indexBuffer[i + 1];
		    m_indexBuffer[i + 1] = m_indexBuffer[i + 2];
		    m_indexBuffer[i + 2] = swap;
		}

		float *pNormal = 0;
		float *pTangent = 0;

		// Invert normals and tangents.
		for (int i = 0; i < static_cast<int>(m_vertexBuffer.size()); ++i)
		{
		    pNormal = m_vertexBuffer[i].normal;
		    pNormal[0] = -pNormal[0];
		    pNormal[1] = -pNormal[1];
		    pNormal[2] = -pNormal[2];

		    pTangent = m_vertexBuffer[i].tangent;
		    pTangent[0] = -pTangent[0];
		    pTangent[1] = -pTangent[1];
		    pTangent[2] = -pTangent[2];
		}
	}
	//--------------------------------------------------------------------------
	void ModelOBJ::scale(float scaleFactor, float offset[3])
	{
		float *pPosition = 0;

		for (int i = 0; i < static_cast<int>(m_vertexBuffer.size()); ++i)
		{
		    pPosition = m_vertexBuffer[i].position;

		    pPosition[0] += offset[0];
		    pPosition[1] += offset[1];
		    pPosition[2] += offset[2];

		    pPosition[0] *= scaleFactor;
		    pPosition[1] *= scaleFactor;
		    pPosition[2] *= scaleFactor;
		}
	}
	//--------------------------------------------------------------------------
	void ModelOBJ::addTrianglePos(int index, int material, int v0, int v1, int v2)
	{
		Vertex vertex =
		{
		    0.0f, 0.0f, 0.0f,
		    0.0f, 0.0f,
		    0.0f, 0.0f, 0.0f
		};

		m_attributeBuffer[index] = material;

		vertex.position[0] = m_vertexCoords[v0 * 3];
		vertex.position[1] = m_vertexCoords[v0 * 3 + 1];
		vertex.position[2] = m_vertexCoords[v0 * 3 + 2];
		m_indexBuffer[index * 3] = addVertex(v0, &vertex);

		vertex.position[0] = m_vertexCoords[v1 * 3];
		vertex.position[1] = m_vertexCoords[v1 * 3 + 1];
		vertex.position[2] = m_vertexCoords[v1 * 3 + 2];
		m_indexBuffer[index * 3 + 1] = addVertex(v1, &vertex);

		vertex.position[0] = m_vertexCoords[v2 * 3];
		vertex.position[1] = m_vertexCoords[v2 * 3 + 1];
		vertex.position[2] = m_vertexCoords[v2 * 3 + 2];
		m_indexBuffer[index * 3 + 2] = addVertex(v2, &vertex);
	}
	//--------------------------------------------------------------------------
	void ModelOBJ::addTrianglePosNormal(int index, int material, int v0, int v1,
		                                int v2, int vn0, int vn1, int vn2)
	{
		Vertex vertex =
		{
		    0.0f, 0.0f, 0.0f,
		    0.0f, 0.0f,
		    0.0f, 0.0f, 0.0f,
		    0.0f, 0.0f, 0.0f
		};

		m_attributeBuffer[index] = material;

		vertex.position[0] = m_vertexCoords[v0 * 3];
		vertex.position[1] = m_vertexCoords[v0 * 3 + 1];
		vertex.position[2] = m_vertexCoords[v0 * 3 + 2];
		vertex.normal[0] = m_normals[vn0 * 3];
		vertex.normal[1] = m_normals[vn0 * 3 + 1];
		vertex.normal[2] = m_normals[vn0 * 3 + 2];
		m_indexBuffer[index * 3] = addVertex(v0, &vertex);

		vertex.position[0] = m_vertexCoords[v1 * 3];
		vertex.position[1] = m_vertexCoords[v1 * 3 + 1];
		vertex.position[2] = m_vertexCoords[v1 * 3 + 2];
		vertex.normal[0] = m_normals[vn1 * 3];
		vertex.normal[1] = m_normals[vn1 * 3 + 1];
		vertex.normal[2] = m_normals[vn1 * 3 + 2];
		m_indexBuffer[index * 3 + 1] = addVertex(v1, &vertex);

		vertex.position[0] = m_vertexCoords[v2 * 3];
		vertex.position[1] = m_vertexCoords[v2 * 3 + 1];
		vertex.position[2] = m_vertexCoords[v2 * 3 + 2];
		vertex.normal[0] = m_normals[vn2 * 3];
		vertex.normal[1] = m_normals[vn2 * 3 + 1];
		vertex.normal[2] = m_normals[vn2 * 3 + 2];
		m_indexBuffer[index * 3 + 2] = addVertex(v2, &vertex);
	}
	//--------------------------------------------------------------------------
	void ModelOBJ::addTrianglePosTexCoord(int index, int material, int v0, int v1,
		                                  int v2, int vt0, int vt1, int vt2)
	{
		Vertex vertex =
		{
		    0.0f, 0.0f, 0.0f,
		    0.0f, 0.0f,
		    0.0f, 0.0f, 0.0f,
		    0.0f, 0.0f, 0.0f
		};

		m_attributeBuffer[index] = material;

		vertex.position[0] = m_vertexCoords[v0 * 3];
		vertex.position[1] = m_vertexCoords[v0 * 3 + 1];
		vertex.position[2] = m_vertexCoords[v0 * 3 + 2];
		vertex.texCoord[0] = m_textureCoords[vt0 * 2];
		vertex.texCoord[1] = m_textureCoords[vt0 * 2 + 1];
		m_indexBuffer[index * 3] = addVertex(v0, &vertex);

		vertex.position[0] = m_vertexCoords[v1 * 3];
		vertex.position[1] = m_vertexCoords[v1 * 3 + 1];
		vertex.position[2] = m_vertexCoords[v1 * 3 + 2];
		vertex.texCoord[0] = m_textureCoords[vt1 * 2];
		vertex.texCoord[1] = m_textureCoords[vt1 * 2 + 1];
		m_indexBuffer[index * 3 + 1] = addVertex(v1, &vertex);

		vertex.position[0] = m_vertexCoords[v2 * 3];
		vertex.position[1] = m_vertexCoords[v2 * 3 + 1];
		vertex.position[2] = m_vertexCoords[v2 * 3 + 2];
		vertex.texCoord[0] = m_textureCoords[vt2 * 2];
		vertex.texCoord[1] = m_textureCoords[vt2 * 2 + 1];
		m_indexBuffer[index * 3 + 2] = addVertex(v2, &vertex);
	}
	//--------------------------------------------------------------------------
	void ModelOBJ::addTrianglePosTexCoordNormal(int index, int material, int v0,
		                                        int v1, int v2, int vt0, int vt1,
		                                        int vt2, int vn0, int vn1, int vn2)
	{
		Vertex vertex =
		{
		    0.0f, 0.0f, 0.0f,
		    0.0f, 0.0f,
		    0.0f, 0.0f, 0.0f,
		    0.0f, 0.0f, 0.0f
		};

		m_attributeBuffer[index] = material;

		vertex.position[0] = m_vertexCoords[v0 * 3];
		vertex.position[1] = m_vertexCoords[v0 * 3 + 1];
		vertex.position[2] = m_vertexCoords[v0 * 3 + 2];
		vertex.texCoord[0] = m_textureCoords[vt0 * 2];
		vertex.texCoord[1] = m_textureCoords[vt0 * 2 + 1];
		vertex.normal[0] = m_normals[vn0 * 3];
		vertex.normal[1] = m_normals[vn0 * 3 + 1];
		vertex.normal[2] = m_normals[vn0 * 3 + 2];
		m_indexBuffer[index * 3] = addVertex(v0, &vertex);

		vertex.position[0] = m_vertexCoords[v1 * 3];
		vertex.position[1] = m_vertexCoords[v1 * 3 + 1];
		vertex.position[2] = m_vertexCoords[v1 * 3 + 2];
		vertex.texCoord[0] = m_textureCoords[vt1 * 2];
		vertex.texCoord[1] = m_textureCoords[vt1 * 2 + 1];
		vertex.normal[0] = m_normals[vn1 * 3];
		vertex.normal[1] = m_normals[vn1 * 3 + 1];
		vertex.normal[2] = m_normals[vn1 * 3 + 2];
		m_indexBuffer[index * 3 + 1] = addVertex(v1, &vertex);

		vertex.position[0] = m_vertexCoords[v2 * 3];
		vertex.position[1] = m_vertexCoords[v2 * 3 + 1];
		vertex.position[2] = m_vertexCoords[v2 * 3 + 2];
		vertex.texCoord[0] = m_textureCoords[vt2 * 2];
		vertex.texCoord[1] = m_textureCoords[vt2 * 2 + 1];
		vertex.normal[0] = m_normals[vn2 * 3];
		vertex.normal[1] = m_normals[vn2 * 3 + 1];
		vertex.normal[2] = m_normals[vn2 * 3 + 2];
		m_indexBuffer[index * 3 + 2] = addVertex(v2, &vertex);
	}
	//--------------------------------------------------------------------------
	int ModelOBJ::addVertex(int hash, const Vertex *pVertex)
	{
		int index = -1;
		std::map<int, std::vector<int> >::const_iterator iter = m_vertexCache.find(hash);

		if (iter == m_vertexCache.end())
		{
		    // Vertex hash doesn't exist in the cache.

		    index = static_cast<int>(m_vertexBuffer.size());
		    m_vertexBuffer.push_back(*pVertex);
		    m_vertexCache.insert(std::make_pair(hash, std::vector<int>(1, index)));
		}
		else
		{
		    // One or more vertices have been hashed to this entry in the cache.

		    const std::vector<int> &vertices = iter->second;
		    const Vertex *pCachedVertex = 0;
		    bool found = false;

		    for (std::vector<int>::const_iterator i = vertices.begin(); i != vertices.end(); ++i)
		    {
		        index = *i;
		        pCachedVertex = &m_vertexBuffer[index];

		        if (memcmp(pCachedVertex, pVertex, sizeof(Vertex)) == 0)
		        {
		            found = true;
		            break;
		        }
		    }

		    if (!found)
		    {
		        index = static_cast<int>(m_vertexBuffer.size());
		        m_vertexBuffer.push_back(*pVertex);
		        m_vertexCache[hash].push_back(index);
		    }
		}

		return index;
	}
	//--------------------------------------------------------------------------
	void ModelOBJ::buildMeshes()
	{
		// Group the model's triangles based on material type.

		Mesh *pMesh = 0;
		int materialId = -1;
		int numMeshes = 0;

		// Count the number of meshes.
		for (int i = 0; i < static_cast<int>(m_attributeBuffer.size()); ++i)
		{
		    if (m_attributeBuffer[i] != materialId)
		    {
		        materialId = m_attributeBuffer[i];
		        ++numMeshes;
		    }
		}

		// Allocate memory for the meshes and reset counters.
		m_numberOfMeshes = numMeshes;
		m_meshes.resize(m_numberOfMeshes);
		numMeshes = 0;
		materialId = -1;

		// Build the meshes. One mesh for each unique material.
		for (int i = 0; i < static_cast<int>(m_attributeBuffer.size()); ++i)
		{
		    if (m_attributeBuffer[i] != materialId)
		    {
		        materialId = m_attributeBuffer[i];
		        pMesh = &m_meshes[numMeshes++];            
		        pMesh->pMaterial = &m_materials[materialId];
		        pMesh->startIndex = i * 3;
		        ++pMesh->triangleCount;
		    }
		    else
		    {
		        ++pMesh->triangleCount;
		    }
		}

		// Sort the meshes based on its material alpha. Fully opaque meshes
		// towards the front and fully transparent towards the back.
		std::sort(m_meshes.begin(), m_meshes.end(), MeshCompFunc);
	}
	//--------------------------------------------------------------------------
	void ModelOBJ::generateNormals()
	{
		const int *pTriangle = 0;
		Vertex *pVertex0 = 0;
		Vertex *pVertex1 = 0;
		Vertex *pVertex2 = 0;
		float edge1[3] = {0.0f, 0.0f, 0.0f};
		float edge2[3] = {0.0f, 0.0f, 0.0f};
		float normal[3] = {0.0f, 0.0f, 0.0f};
		float length = 0.0f;
		int totalVertices = getNumberOfVertices();
		int totalTriangles = getNumberOfTriangles();

		// Initialize all the vertex normals.
		for (int i = 0; i < totalVertices; ++i)
		{
		    pVertex0 = &m_vertexBuffer[i];
		    pVertex0->normal[0] = 0.0f;
		    pVertex0->normal[1] = 0.0f;
		    pVertex0->normal[2] = 0.0f;
		}

		// Calculate the vertex normals.
		for (int i = 0; i < totalTriangles; ++i)
		{
		    pTriangle = &m_indexBuffer[i * 3];

		    pVertex0 = &m_vertexBuffer[pTriangle[0]];
		    pVertex1 = &m_vertexBuffer[pTriangle[1]];
		    pVertex2 = &m_vertexBuffer[pTriangle[2]];

		    // Calculate triangle face normal.

		    edge1[0] = pVertex1->position[0] - pVertex0->position[0];
		    edge1[1] = pVertex1->position[1] - pVertex0->position[1];
		    edge1[2] = pVertex1->position[2] - pVertex0->position[2];

		    edge2[0] = pVertex2->position[0] - pVertex0->position[0];
		    edge2[1] = pVertex2->position[1] - pVertex0->position[1];
		    edge2[2] = pVertex2->position[2] - pVertex0->position[2];

		    normal[0] = (edge1[1] * edge2[2]) - (edge1[2] * edge2[1]);
		    normal[1] = (edge1[2] * edge2[0]) - (edge1[0] * edge2[2]);
		    normal[2] = (edge1[0] * edge2[1]) - (edge1[1] * edge2[0]);

		    // Accumulate the normals.

		    pVertex0->normal[0] += normal[0];
		    pVertex0->normal[1] += normal[1];
		    pVertex0->normal[2] += normal[2];

		    pVertex1->normal[0] += normal[0];
		    pVertex1->normal[1] += normal[1];
		    pVertex1->normal[2] += normal[2];

		    pVertex2->normal[0] += normal[0];
		    pVertex2->normal[1] += normal[1];
		    pVertex2->normal[2] += normal[2];
		}

		// Normalize the vertex normals.
		for (int i = 0; i < totalVertices; ++i)
		{
		    pVertex0 = &m_vertexBuffer[i];

		    length = 1.0f / sqrtf(pVertex0->normal[0] * pVertex0->normal[0] +
		        pVertex0->normal[1] * pVertex0->normal[1] +
		        pVertex0->normal[2] * pVertex0->normal[2]);

		    pVertex0->normal[0] *= length;
		    pVertex0->normal[1] *= length;
		    pVertex0->normal[2] *= length;
		}

		m_hasNormals = true;
	}
	//--------------------------------------------------------------------------
	void ModelOBJ::generateTangents()
	{
		const int *pTriangle = 0;
		Vertex *pVertex0 = 0;
		Vertex *pVertex1 = 0;
		Vertex *pVertex2 = 0;
		float edge1[3] = {0.0f, 0.0f, 0.0f};
		float edge2[3] = {0.0f, 0.0f, 0.0f};
		float texEdge1[2] = {0.0f, 0.0f};
		float texEdge2[2] = {0.0f, 0.0f};
		float tangent[3] = {0.0f, 0.0f, 0.0f};
		float bitangent[3] = {0.0f, 0.0f, 0.0f};
		float det = 0.0f;
		float nDotT = 0.0f;
		float bDotB = 0.0f;
		float length = 0.0f;
		int totalVertices = getNumberOfVertices();
		int totalTriangles = getNumberOfTriangles();

		// Initialize all the vertex tangents and bitangents.
		for (int i = 0; i < totalVertices; ++i)
		{
		    pVertex0 = &m_vertexBuffer[i];

		    pVertex0->tangent[0] = 0.0f;
		    pVertex0->tangent[1] = 0.0f;
		    pVertex0->tangent[2] = 0.0f;
		    pVertex0->tangent[3] = 0.0f;

		    pVertex0->bitangent[0] = 0.0f;
		    pVertex0->bitangent[1] = 0.0f;
		    pVertex0->bitangent[2] = 0.0f;
		}

		// Calculate the vertex tangents and bitangents.
		for (int i = 0; i < totalTriangles; ++i)
		{
		    pTriangle = &m_indexBuffer[i * 3];

		    pVertex0 = &m_vertexBuffer[pTriangle[0]];
		    pVertex1 = &m_vertexBuffer[pTriangle[1]];
		    pVertex2 = &m_vertexBuffer[pTriangle[2]];

		    // Calculate the triangle face tangent and bitangent.

		    edge1[0] = pVertex1->position[0] - pVertex0->position[0];
		    edge1[1] = pVertex1->position[1] - pVertex0->position[1];
		    edge1[2] = pVertex1->position[2] - pVertex0->position[2];

		    edge2[0] = pVertex2->position[0] - pVertex0->position[0];
		    edge2[1] = pVertex2->position[1] - pVertex0->position[1];
		    edge2[2] = pVertex2->position[2] - pVertex0->position[2];

		    texEdge1[0] = pVertex1->texCoord[0] - pVertex0->texCoord[0];
		    texEdge1[1] = pVertex1->texCoord[1] - pVertex0->texCoord[1];

		    texEdge2[0] = pVertex2->texCoord[0] - pVertex0->texCoord[0];
		    texEdge2[1] = pVertex2->texCoord[1] - pVertex0->texCoord[1];

		    det = texEdge1[0] * texEdge2[1] - texEdge2[0] * texEdge1[1];

		    if (fabs(det) < 1e-6f)
		    {
		        tangent[0] = 1.0f;
		        tangent[1] = 0.0f;
		        tangent[2] = 0.0f;

		        bitangent[0] = 0.0f;
		        bitangent[1] = 1.0f;
		        bitangent[2] = 0.0f;
		    }
		    else
		    {
		        det = 1.0f / det;

		        tangent[0] = (texEdge2[1] * edge1[0] - texEdge1[1] * edge2[0]) * det;
		        tangent[1] = (texEdge2[1] * edge1[1] - texEdge1[1] * edge2[1]) * det;
		        tangent[2] = (texEdge2[1] * edge1[2] - texEdge1[1] * edge2[2]) * det;

		        bitangent[0] = (-texEdge2[0] * edge1[0] + texEdge1[0] * edge2[0]) * det;
		        bitangent[1] = (-texEdge2[0] * edge1[1] + texEdge1[0] * edge2[1]) * det;
		        bitangent[2] = (-texEdge2[0] * edge1[2] + texEdge1[0] * edge2[2]) * det;
		    }

		    // Accumulate the tangents and bitangents.

		    pVertex0->tangent[0] += tangent[0];
		    pVertex0->tangent[1] += tangent[1];
		    pVertex0->tangent[2] += tangent[2];
		    pVertex0->bitangent[0] += bitangent[0];
		    pVertex0->bitangent[1] += bitangent[1];
		    pVertex0->bitangent[2] += bitangent[2];

		    pVertex1->tangent[0] += tangent[0];
		    pVertex1->tangent[1] += tangent[1];
		    pVertex1->tangent[2] += tangent[2];
		    pVertex1->bitangent[0] += bitangent[0];
		    pVertex1->bitangent[1] += bitangent[1];
		    pVertex1->bitangent[2] += bitangent[2];

		    pVertex2->tangent[0] += tangent[0];
		    pVertex2->tangent[1] += tangent[1];
		    pVertex2->tangent[2] += tangent[2];
		    pVertex2->bitangent[0] += bitangent[0];
		    pVertex2->bitangent[1] += bitangent[1];
		    pVertex2->bitangent[2] += bitangent[2];
		}

		// Orthogonalize and normalize the vertex tangents.
		for (int i = 0; i < totalVertices; ++i)
		{
		    pVertex0 = &m_vertexBuffer[i];

		    // Gram-Schmidt orthogonalize tangent with normal.

		    nDotT = pVertex0->normal[0] * pVertex0->tangent[0] +
		            pVertex0->normal[1] * pVertex0->tangent[1] +
		            pVertex0->normal[2] * pVertex0->tangent[2];

		    pVertex0->tangent[0] -= pVertex0->normal[0] * nDotT;
		    pVertex0->tangent[1] -= pVertex0->normal[1] * nDotT;
		    pVertex0->tangent[2] -= pVertex0->normal[2] * nDotT;

		    // Normalize the tangent.

		    length = 1.0f / sqrtf(pVertex0->tangent[0] * pVertex0->tangent[0] +
		                          pVertex0->tangent[1] * pVertex0->tangent[1] +
		                          pVertex0->tangent[2] * pVertex0->tangent[2]);

		    pVertex0->tangent[0] *= length;
		    pVertex0->tangent[1] *= length;
		    pVertex0->tangent[2] *= length;

		    // Calculate the handedness of the local tangent space.
		    // The bitangent vector is the cross product between the triangle face
		    // normal vector and the calculated tangent vector. The resulting
		    // bitangent vector should be the same as the bitangent vector
		    // calculated from the set of linear equations above. If they point in
		    // different directions then we need to invert the cross product
		    // calculated bitangent vector. We store this scalar multiplier in the
		    // tangent vector's 'w' component so that the correct bitangent vector
		    // can be generated in the normal mapping shader's vertex shader.
		    //
		    // Normal maps have a left handed coordinate system with the origin
		    // located at the top left of the normal map texture. The x coordinates
		    // run horizontally from left to right. The y coordinates run
		    // vertically from top to bottom. The z coordinates run out of the
		    // normal map texture towards the viewer. Our handedness calculations
		    // must take this fact into account as well so that the normal mapping
		    // shader's vertex shader will generate the correct bitangent vectors.
		    // Some normal map authoring tools such as Crazybump
		    // (http://www.crazybump.com/) includes options to allow you to control
		    // the orientation of the normal map normal's y-axis.

		    bitangent[0] = (pVertex0->normal[1] * pVertex0->tangent[2]) - 
		                   (pVertex0->normal[2] * pVertex0->tangent[1]);
		    bitangent[1] = (pVertex0->normal[2] * pVertex0->tangent[0]) -
		                   (pVertex0->normal[0] * pVertex0->tangent[2]);
		    bitangent[2] = (pVertex0->normal[0] * pVertex0->tangent[1]) - 
		                   (pVertex0->normal[1] * pVertex0->tangent[0]);

		    bDotB = bitangent[0] * pVertex0->bitangent[0] + 
		            bitangent[1] * pVertex0->bitangent[1] + 
		            bitangent[2] * pVertex0->bitangent[2];

		    pVertex0->tangent[3] = (bDotB < 0.0f) ? 1.0f : -1.0f;

		    pVertex0->bitangent[0] = bitangent[0];
		    pVertex0->bitangent[1] = bitangent[1];
		    pVertex0->bitangent[2] = bitangent[2];
		}

		m_hasTangents = true;
	}
	//--------------------------------------------------------------------------
	void ModelOBJ::importGeometryFirstPass(FILE *pFile)
	{
		m_hasTextureCoords = false;
		m_hasNormals = false;

		m_numberOfVertexCoords = 0;
		m_numberOfTextureCoords = 0;
		m_numberOfNormals = 0;
		m_numberOfTriangles = 0;

		int v = 0;
		int vt = 0;
		int vn = 0;
		char buffer[256] = {0};
		std::string name;

		while (fscanf(pFile, "%s", buffer) != EOF)
		{
		    switch (buffer[0])
		    {
		    case 'f':   // v, v//vn, v/vt, v/vt/vn.
		        fscanf(pFile, "%s", buffer);

		        if (strstr(buffer, "//")) // v//vn
		        {
		            sscanf(buffer, "%d//%d", &v, &vn);
		            fscanf(pFile, "%d//%d", &v, &vn);
		            fscanf(pFile, "%d//%d", &v, &vn);
		            ++m_numberOfTriangles;

		            while (fscanf(pFile, "%d//%d", &v, &vn) > 0)
		                ++m_numberOfTriangles;
		        }
		        else if (sscanf(buffer, "%d/%d/%d", &v, &vt, &vn) == 3) // v/vt/vn
		        {
		            fscanf(pFile, "%d/%d/%d", &v, &vt, &vn);
		            fscanf(pFile, "%d/%d/%d", &v, &vt, &vn);
		            ++m_numberOfTriangles;

		            while (fscanf(pFile, "%d/%d/%d", &v, &vt, &vn) > 0)
		                ++m_numberOfTriangles;
		        }
		        else if (sscanf(buffer, "%d/%d", &v, &vt) == 2) // v/vt
		        {
		            fscanf(pFile, "%d/%d", &v, &vt);
		            fscanf(pFile, "%d/%d", &v, &vt);
		            ++m_numberOfTriangles;

		            while (fscanf(pFile, "%d/%d", &v, &vt) > 0)
		                ++m_numberOfTriangles;
		        }
		        else // v
		        {
		            fscanf(pFile, "%d", &v);
		            fscanf(pFile, "%d", &v);
		            ++m_numberOfTriangles;

		            while (fscanf(pFile, "%d", &v) > 0)
		                ++m_numberOfTriangles;
		        }
		        break;

		    case 'm':   // mtllib
		        fgets(buffer, sizeof(buffer), pFile);
		        sscanf(buffer, "%s %s", buffer, buffer);
		        name = m_directoryPath;
		        name += buffer;
		        importMaterials(name.c_str());
		        break;

		    case 'v':   // v, vt, or vn
		        switch (buffer[1])
		        {
		        case '\0':
		            fgets(buffer, sizeof(buffer), pFile);
		            ++m_numberOfVertexCoords;
		            break;

		        case 'n':
		            fgets(buffer, sizeof(buffer), pFile);
		            ++m_numberOfNormals;
		            break;

		        case 't':
		            fgets(buffer, sizeof(buffer), pFile);
		            ++m_numberOfTextureCoords;

		        default:
		            break;
		        }
		        break;

		    default:
		        fgets(buffer, sizeof(buffer), pFile);
		        break;
		    }
		}

		m_hasPositions = m_numberOfVertexCoords > 0;
		m_hasNormals = m_numberOfNormals > 0;
		m_hasTextureCoords = m_numberOfTextureCoords > 0;

		// Allocate memory for the OBJ model data.
		m_vertexCoords.resize(m_numberOfVertexCoords * 3);
		m_textureCoords.resize(m_numberOfTextureCoords * 2);
		m_normals.resize(m_numberOfNormals * 3);
		m_indexBuffer.resize(m_numberOfTriangles * 3);
		m_attributeBuffer.resize(m_numberOfTriangles);

		// Define a default material if no materials were loaded.
		if (m_numberOfMaterials == 0)
		{
		    Material defaultMaterial =
		    {
		        0.2f, 0.2f, 0.2f, 1.0f,
		        0.8f, 0.8f, 0.8f, 1.0f,
		        0.0f, 0.0f, 0.0f, 1.0f,
		        0.0f,
		        1.0f,
		        std::string("default"),
		        std::string(),
		        std::string()
		    };

		    m_materials.push_back(defaultMaterial);
		    m_materialCache[defaultMaterial.name] = 0;
		}
	}
	//--------------------------------------------------------------------------
	void ModelOBJ::importGeometrySecondPass(FILE *pFile)
	{
		int v[3] = {0};
		int vt[3] = {0};
		int vn[3] = {0};
		int numVertices = 0;
		int numTexCoords = 0;
		int numNormals = 0;
		int numTriangles = 0;
		int activeMaterial = 0;
		char buffer[256] = {0};
		std::string name;
		std::map<std::string, int>::const_iterator iter;

		while (fscanf(pFile, "%s", buffer) != EOF)
		{
		    switch (buffer[0])
		    {
		    case 'f': // v, v//vn, v/vt, or v/vt/vn.
		        v[0]  = v[1]  = v[2]  = 0;
		        vt[0] = vt[1] = vt[2] = 0;
		        vn[0] = vn[1] = vn[2] = 0;

		        fscanf(pFile, "%s", buffer);

		        if (strstr(buffer, "//")) // v//vn
		        {
		            sscanf(buffer, "%d//%d", &v[0], &vn[0]);
		            fscanf(pFile, "%d//%d", &v[1], &vn[1]);
		            fscanf(pFile, "%d//%d", &v[2], &vn[2]);

		            v[0] = (v[0] < 0) ? v[0] + numVertices - 1 : v[0] - 1;
		            v[1] = (v[1] < 0) ? v[1] + numVertices - 1 : v[1] - 1;
		            v[2] = (v[2] < 0) ? v[2] + numVertices - 1 : v[2] - 1;

		            vn[0] = (vn[0] < 0) ? vn[0] + numNormals - 1 : vn[0] - 1;
		            vn[1] = (vn[1] < 0) ? vn[1] + numNormals - 1 : vn[1] - 1;
		            vn[2] = (vn[2] < 0) ? vn[2] + numNormals - 1 : vn[2] - 1;

		            addTrianglePosNormal(numTriangles++, activeMaterial,
		                v[0], v[1], v[2], vn[0], vn[1], vn[2]);

		            v[1] = v[2];
		            vn[1] = vn[2];

		            while (fscanf(pFile, "%d//%d", &v[2], &vn[2]) > 0)
		            {
		                v[2] = (v[2] < 0) ? v[2] + numVertices - 1 : v[2] - 1;
		                vn[2] = (vn[2] < 0) ? vn[2] + numNormals - 1 : vn[2] - 1;

		                addTrianglePosNormal(numTriangles++, activeMaterial,
		                    v[0], v[1], v[2], vn[0], vn[1], vn[2]);

		                v[1] = v[2];
		                vn[1] = vn[2];
		            }
		        }
		        else if (sscanf(buffer, "%d/%d/%d", &v[0], &vt[0], &vn[0]) == 3) // v/vt/vn
		        {
		            fscanf(pFile, "%d/%d/%d", &v[1], &vt[1], &vn[1]);
		            fscanf(pFile, "%d/%d/%d", &v[2], &vt[2], &vn[2]);

		            v[0] = (v[0] < 0) ? v[0] + numVertices - 1 : v[0] - 1;
		            v[1] = (v[1] < 0) ? v[1] + numVertices - 1 : v[1] - 1;
		            v[2] = (v[2] < 0) ? v[2] + numVertices - 1 : v[2] - 1;

		            vt[0] = (vt[0] < 0) ? vt[0] + numTexCoords - 1 : vt[0] - 1;
		            vt[1] = (vt[1] < 0) ? vt[1] + numTexCoords - 1 : vt[1] - 1;
		            vt[2] = (vt[2] < 0) ? vt[2] + numTexCoords - 1 : vt[2] - 1;

		            vn[0] = (vn[0] < 0) ? vn[0] + numNormals - 1 : vn[0] - 1;
		            vn[1] = (vn[1] < 0) ? vn[1] + numNormals - 1 : vn[1] - 1;
		            vn[2] = (vn[2] < 0) ? vn[2] + numNormals - 1 : vn[2] - 1;

		            addTrianglePosTexCoordNormal(numTriangles++, activeMaterial,
		                v[0], v[1], v[2], vt[0], vt[1], vt[2], vn[0], vn[1], vn[2]);

		            v[1] = v[2];
		            vt[1] = vt[2];
		            vn[1] = vn[2];

		            while (fscanf(pFile, "%d/%d/%d", &v[2], &vt[2], &vn[2]) > 0)
		            {
		                v[2] = (v[2] < 0) ? v[2] + numVertices - 1 : v[2] - 1;
		                vt[2] = (vt[2] < 0) ? vt[2] + numTexCoords - 1 : vt[2] - 1;
		                vn[2] = (vn[2] < 0) ? vn[2] + numNormals - 1 : vn[2] - 1;

		                addTrianglePosTexCoordNormal(numTriangles++, activeMaterial,
		                    v[0], v[1], v[2], vt[0], vt[1], vt[2], vn[0], vn[1], vn[2]);

		                v[1] = v[2];
		                vt[1] = vt[2];
		                vn[1] = vn[2];
		            }
		        }
		        else if (sscanf(buffer, "%d/%d", &v[0], &vt[0]) == 2) // v/vt
		        {
		            fscanf(pFile, "%d/%d", &v[1], &vt[1]);
		            fscanf(pFile, "%d/%d", &v[2], &vt[2]);

		            v[0] = (v[0] < 0) ? v[0] + numVertices - 1 : v[0] - 1;
		            v[1] = (v[1] < 0) ? v[1] + numVertices - 1 : v[1] - 1;
		            v[2] = (v[2] < 0) ? v[2] + numVertices - 1 : v[2] - 1;

		            vt[0] = (vt[0] < 0) ? vt[0] + numTexCoords - 1 : vt[0] - 1;
		            vt[1] = (vt[1] < 0) ? vt[1] + numTexCoords - 1 : vt[1] - 1;
		            vt[2] = (vt[2] < 0) ? vt[2] + numTexCoords - 1 : vt[2] - 1;

		            addTrianglePosTexCoord(numTriangles++, activeMaterial,
		                v[0], v[1], v[2], vt[0], vt[1], vt[2]);

		            v[1] = v[2];
		            vt[1] = vt[2];

		            while (fscanf(pFile, "%d/%d", &v[2], &vt[2]) > 0)
		            {
		                v[2] = (v[2] < 0) ? v[2] + numVertices - 1 : v[2] - 1;
		                vt[2] = (vt[2] < 0) ? vt[2] + numTexCoords - 1 : vt[2] - 1;

		                addTrianglePosTexCoord(numTriangles++, activeMaterial,
		                    v[0], v[1], v[2], vt[0], vt[1], vt[2]);

		                v[1] = v[2];
		                vt[1] = vt[2];
		            }
		        }
		        else // v
		        {
		            sscanf(buffer, "%d", &v[0]);
		            fscanf(pFile, "%d", &v[1]);
		            fscanf(pFile, "%d", &v[2]);

		            v[0] = (v[0] < 0) ? v[0] + numVertices - 1 : v[0] - 1;
		            v[1] = (v[1] < 0) ? v[1] + numVertices - 1 : v[1] - 1;
		            v[2] = (v[2] < 0) ? v[2] + numVertices - 1 : v[2] - 1;

		            addTrianglePos(numTriangles++, activeMaterial, v[0], v[1], v[2]);

		            v[1] = v[2];

		            while (fscanf(pFile, "%d", &v[2]) > 0)
		            {
		                v[2] = (v[2] < 0) ? v[2] + numVertices - 1 : v[2] - 1;

		                addTrianglePos(numTriangles++, activeMaterial, v[0], v[1], v[2]);

		                v[1] = v[2];
		            }
		        }
		        break;

		    case 'u': // usemtl
		        fgets(buffer, sizeof(buffer), pFile);
		        sscanf(buffer, "%s %s", buffer, buffer);
		        name = buffer;
		        iter = m_materialCache.find(buffer);
		        activeMaterial = (iter == m_materialCache.end()) ? 0 : iter->second;
		        break;

		    case 'v': // v, vn, or vt.
		        switch (buffer[1])
		        {
		        case '\0': // v
		            fscanf(pFile, "%f %f %f",
		                &m_vertexCoords[3 * numVertices],
		                &m_vertexCoords[3 * numVertices + 1],
		                &m_vertexCoords[3 * numVertices + 2]);
		            ++numVertices;
		            break;

		        case 'n': // vn
		            fscanf(pFile, "%f %f %f",
		                &m_normals[3 * numNormals],
		                &m_normals[3 * numNormals + 1],
		                &m_normals[3 * numNormals + 2]);
		            ++numNormals;
		            break;

		        case 't': // vt
		            fscanf(pFile, "%f %f",
		                &m_textureCoords[2 * numTexCoords],
		                &m_textureCoords[2 * numTexCoords + 1]);
		            ++numTexCoords;
		            break;

		        default:
		            break;
		        }
		        break;

		    default:
		        fgets(buffer, sizeof(buffer), pFile);
		        break;
		    }
		}
	}
	//--------------------------------------------------------------------------
	bool ModelOBJ::importMaterials(const char *pszFilename)
	{
		FILE *pFile = fopen(pszFilename, "r");

		if (!pFile)
		    return false;

		Material *pMaterial = 0;
		int illum = 0;
		int numMaterials = 0;
		char buffer[256] = {0};

		// Count the number of materials in the MTL file.
		while (fscanf(pFile, "%s", buffer) != EOF)
		{
		    switch (buffer[0])
		    {
		    case 'n': // newmtl
		        ++numMaterials;
		        fgets(buffer, sizeof(buffer), pFile);
		        sscanf(buffer, "%s %s", buffer, buffer);
		        break;

		    default:
		        fgets(buffer, sizeof(buffer), pFile);
		        break;
		    }
		}

		rewind(pFile);

		m_numberOfMaterials = numMaterials;
		numMaterials = 0;
		m_materials.resize(m_numberOfMaterials);

		// Load the materials in the MTL file.
		while (fscanf(pFile, "%s", buffer) != EOF)
		{
		    switch (buffer[0])
		    {
		    case 'N': // Ns
		        fscanf(pFile, "%f", &pMaterial->shininess);

		        // Wavefront .MTL file shininess is from [0,1000].
		        // Scale back to a generic [0,1] range.
		        pMaterial->shininess /= 1000.0f;
		        break;

		    case 'K': // Ka, Kd, or Ks
		        switch (buffer[1])
		        {
		        case 'a': // Ka
		            fscanf(pFile, "%f %f %f",
		                &pMaterial->ambient[0],
		                &pMaterial->ambient[1],
		                &pMaterial->ambient[2]);
		            pMaterial->ambient[3] = 1.0f;
		            break;

		        case 'd': // Kd
		            fscanf(pFile, "%f %f %f",
		                &pMaterial->diffuse[0],
		                &pMaterial->diffuse[1],
		                &pMaterial->diffuse[2]);
		            pMaterial->diffuse[3] = 1.0f;
		            break;

		        case 's': // Ks
		            fscanf(pFile, "%f %f %f",
		                &pMaterial->specular[0],
		                &pMaterial->specular[1],
		                &pMaterial->specular[2]);
		            pMaterial->specular[3] = 1.0f;
		            break;

		        default:
		            fgets(buffer, sizeof(buffer), pFile);
		            break;
		        }
		        break;

		    case 'T': // Tr
		        switch (buffer[1])
		        {
		        case 'r': // Tr
		            fscanf(pFile, "%f", &pMaterial->alpha);
		            pMaterial->alpha = 1.0f - pMaterial->alpha;
		            break;

		        default:
		            fgets(buffer, sizeof(buffer), pFile);
		            break;
		        }
		        break;

		    case 'd':
		        fscanf(pFile, "%f", &pMaterial->alpha);
		        break;

		    case 'i': // illum
		        fscanf(pFile, "%d", &illum);

		        if (illum == 1)
		        {
		            pMaterial->specular[0] = 0.0f;
		            pMaterial->specular[1] = 0.0f;
		            pMaterial->specular[2] = 0.0f;
		            pMaterial->specular[3] = 1.0f;
		        }
		        break;

		    case 'm': // map_Kd, map_bump
		        if (strstr(buffer, "map_Kd") != 0)
		        {
		            fgets(buffer, sizeof(buffer), pFile);
		            sscanf(buffer, "%s %s", buffer, buffer);
		            pMaterial->colorMapFilename = buffer;
		        }
		        else if (strstr(buffer, "map_bump") != 0)
		        {
		            fgets(buffer, sizeof(buffer), pFile);
		            sscanf(buffer, "%s %s", buffer, buffer);
		            pMaterial->bumpMapFilename = buffer;
		        }
		        else
		        {
		            fgets(buffer, sizeof(buffer), pFile);
		        }
		        break;

		    case 'n': // newmtl
		        fgets(buffer, sizeof(buffer), pFile);
		        sscanf(buffer, "%s %s", buffer, buffer);

		        pMaterial = &m_materials[numMaterials];
		        pMaterial->ambient[0] = 0.2f;
		        pMaterial->ambient[1] = 0.2f;
		        pMaterial->ambient[2] = 0.2f;
		        pMaterial->ambient[3] = 1.0f;
		        pMaterial->diffuse[0] = 0.8f;
		        pMaterial->diffuse[1] = 0.8f;
		        pMaterial->diffuse[2] = 0.8f;
		        pMaterial->diffuse[3] = 1.0f;
		        pMaterial->specular[0] = 0.0f;
		        pMaterial->specular[1] = 0.0f;
		        pMaterial->specular[2] = 0.0f;
		        pMaterial->specular[3] = 1.0f;
		        pMaterial->shininess = 0.0f;
		        pMaterial->alpha = 1.0f;
		        pMaterial->name = buffer;
		        pMaterial->colorMapFilename.clear();
		        pMaterial->bumpMapFilename.clear();

		        m_materialCache[pMaterial->name] = numMaterials;
		        ++numMaterials;
		        break;

		    default:
		        fgets(buffer, sizeof(buffer), pFile);
		        break;
		    }
		}

		fclose(pFile);
		return true;
	}
	//--------------------------------------------------------------------------
}


// JSON Loader
namespace 
{
	//--------------------------------------------------------------------------
	//	Copyright (c) 2009 Dave Gamble
	//
	//	Permission is hereby granted, free of charge, to any person obtaining a copy
	//	of this software and associated documentation files (the "Software"), to deal
	//	in the Software without restriction, including without limitation the rights
	//	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
	//	copies of the Software, and to permit persons to whom the Software is
	//	furnished to do so, subject to the following conditions:
	//
	//	The above copyright notice and this permission notice shall be included in
	//	all copies or substantial portions of the Software.
	//
	//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
	//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
	//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
	//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
	//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
	//	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
	//	THE SOFTWARE.
	//--------------------------------------------------------------------------
	extern "C"
	{
		/* The cJSON structure: */
		typedef struct cJSON 
		{
			struct cJSON *next,*prev;	/* next/prev allow you to walk array/object chains. Alternatively, use GetArraySize/GetArrayItem/GetObjectItem */
			struct cJSON *child;		/* An array or object item will have a child pointer pointing to a chain of the items in the array/object. */

			int type;					/* The type of the item, as above. */

			char *valuestring;			/* The item's string, if type==cJSON_String */
			int valueint;				/* The item's number, if type==cJSON_Number */
			double valuedouble;			/* The item's number, if type==cJSON_Number */

			char *string;				/* The item's name string, if this item is the child of, or is in the list of subitems of an object. */
		} cJSON;

		typedef struct cJSON_Hooks 
		{
			  void *(*malloc_fn)(size_t sz);
			  void (*free_fn)(void *ptr);
		} cJSON_Hooks;

		/* Supply malloc, realloc and free functions to cJSON */
		extern void cJSON_InitHooks(cJSON_Hooks* hooks);


		/* Supply a block of JSON, and this returns a cJSON object you can interrogate. Call cJSON_Delete when finished. */
		extern cJSON *cJSON_Parse(const char *value);
		/* Render a cJSON entity to text for transfer/storage. Free the char* when finished. */
		extern char  *cJSON_Print(cJSON *item);
		/* Render a cJSON entity to text for transfer/storage without any formatting. Free the char* when finished. */
		extern char  *cJSON_PrintUnformatted(cJSON *item);
		/* Delete a cJSON entity and all subentities. */
		extern void   cJSON_Delete(cJSON *c);

		/* Returns the number of items in an array (or object). */
		extern int	  cJSON_GetArraySize(cJSON *array);
		/* Retrieve item number "item" from array "array". Returns NULL if unsuccessful. */
		extern cJSON *cJSON_GetArrayItem(cJSON *array,int item);
		/* Get item "string" from object. Case insensitive. */
		extern cJSON *cJSON_GetObjectItem(cJSON *object,const char *string);

		/* For analysing failed parses. This returns a pointer to the parse error. You'll probably need to look a few chars back to make sense of it. Defined when cJSON_Parse() returns 0. 0 when cJSON_Parse() succeeds. */
		extern const char *cJSON_GetErrorPtr();
	
		/* These calls create a cJSON item of the appropriate type. */
		extern cJSON *cJSON_CreateNull();
		extern cJSON *cJSON_CreateTrue();
		extern cJSON *cJSON_CreateFalse();
		extern cJSON *cJSON_CreateBool(int b);
		extern cJSON *cJSON_CreateNumber(double num);
		extern cJSON *cJSON_CreateString(const char *string);
		extern cJSON *cJSON_CreateArray();
		extern cJSON *cJSON_CreateObject();

		/* These utilities create an Array of count items. */
		extern cJSON *cJSON_CreateIntArray(int *numbers,int count);
		extern cJSON *cJSON_CreateFloatArray(float *numbers,int count);
		extern cJSON *cJSON_CreateDoubleArray(double *numbers,int count);
		extern cJSON *cJSON_CreateStringArray(const char **strings,int count);

		/* Append item to the specified array/object. */
		extern void cJSON_AddItemToArray(cJSON *array, cJSON *item);
		extern void	cJSON_AddItemToObject(cJSON *object,const char *string,cJSON *item);
		/* Append reference to item to the specified array/object. Use this when you want to add an existing cJSON to a new cJSON, but don't want to corrupt your existing cJSON. */
		extern void cJSON_AddItemReferenceToArray(cJSON *array, cJSON *item);
		extern void	cJSON_AddItemReferenceToObject(cJSON *object,const char *string,cJSON *item);

		/* Remove/Detatch items from Arrays/Objects. */
		extern cJSON *cJSON_DetachItemFromArray(cJSON *array,int which);
		extern void   cJSON_DeleteItemFromArray(cJSON *array,int which);
		extern cJSON *cJSON_DetachItemFromObject(cJSON *object,const char *string);
		extern void   cJSON_DeleteItemFromObject(cJSON *object,const char *string);
	
		/* Update array items. */
		extern void cJSON_ReplaceItemInArray(cJSON *array,int which,cJSON *newitem);
		extern void cJSON_ReplaceItemInObject(cJSON *object,const char *string,cJSON *newitem);

		#define cJSON_AddNullToObject(object,name)	cJSON_AddItemToObject(object, name, cJSON_CreateNull())
		#define cJSON_AddTrueToObject(object,name)	cJSON_AddItemToObject(object, name, cJSON_CreateTrue())
		#define cJSON_AddFalseToObject(object,name)		cJSON_AddItemToObject(object, name, cJSON_CreateFalse())
		#define cJSON_AddNumberToObject(object,name,n)	cJSON_AddItemToObject(object, name, cJSON_CreateNumber(n))
		#define cJSON_AddStringToObject(object,name,s)	cJSON_AddItemToObject(object, name, cJSON_CreateString(s))
	}

	static const char *ep;

	const char *cJSON_GetErrorPtr() {return ep;}

	static int cJSON_strcasecmp(const char *s1,const char *s2)
	{
		if (!s1) return (s1==s2)?0:1;if (!s2) return 1;
		for(; tolower(*s1) == tolower(*s2); ++s1, ++s2)	if(*s1 == 0)	return 0;
		return tolower(*(const unsigned char *)s1) - tolower(*(const unsigned char *)s2);
	}

	static void *(*cJSON_malloc)(size_t sz) = malloc;
	static void (*cJSON_free)(void *ptr) = free;

	static char* cJSON_strdup(const char* str)
	{
		  size_t len;
		  char* copy;

		  len = strlen(str) + 1;
		  if (!(copy = (char*)cJSON_malloc(len))) return 0;
		  memcpy(copy,str,len);
		  return copy;
	}

	void cJSON_InitHooks(cJSON_Hooks* hooks)
	{
		if (!hooks) { /* Reset hooks */
		    cJSON_malloc = malloc;
		    cJSON_free = free;
		    return;
		}

		cJSON_malloc = (hooks->malloc_fn)?hooks->malloc_fn:malloc;
		cJSON_free	 = (hooks->free_fn)?hooks->free_fn:free;
	}

	/* Internal constructor. */
	static cJSON *cJSON_New_Item()
	{
		cJSON* node = (cJSON*)cJSON_malloc(sizeof(cJSON));
		if (node) memset(node,0,sizeof(cJSON));
		return node;
	}

	/* Delete a cJSON structure. */
	void cJSON_Delete(cJSON *c)
	{
		cJSON *next;
		while (c)
		{
			next=c->next;
			if (!(c->type&cJSON_IsReference) && c->child) cJSON_Delete(c->child);
			if (!(c->type&cJSON_IsReference) && c->valuestring) cJSON_free(c->valuestring);
			if (c->string) cJSON_free(c->string);
			cJSON_free(c);
			c=next;
		}
	}

	/* Parse the input text to generate a number, and populate the result into item. */
	static const char *parse_number(cJSON *item,const char *num)
	{
		double n=0,sign=1,scale=0;int subscale=0,signsubscale=1;

		/* Could use sscanf for this? */
		if (*num=='-') sign=-1,num++;	/* Has sign? */
		if (*num=='0') num++;			/* is zero */
		if (*num>='1' && *num<='9')	do	n=(n*10.0)+(*num++ -'0');	while (*num>='0' && *num<='9');	/* Number? */
		if (*num=='.') {num++;		do	n=(n*10.0)+(*num++ -'0'),scale--; while (*num>='0' && *num<='9');}	/* Fractional part? */
		if (*num=='e' || *num=='E')		/* Exponent? */
		{	num++;if (*num=='+') num++;	else if (*num=='-') signsubscale=-1,num++;		/* With sign? */
			while (*num>='0' && *num<='9') subscale=(subscale*10)+(*num++ - '0');	/* Number? */
		}

		n=sign*n*pow(10.0,(scale+subscale*signsubscale));	/* number = +/- number.fraction * 10^+/- exponent */
	
		item->valuedouble=n;
		item->valueint=(int)n;
		item->type=cJSON_Number;
		return num;
	}

	/* Render the number nicely from the given item into a string. */
	static char *print_number(cJSON *item)
	{
		char *str;
		double d=item->valuedouble;
		if (fabs(((double)item->valueint)-d)<=DBL_EPSILON && d<=INT_MAX && d>=INT_MIN)
		{
			str=(char*)cJSON_malloc(21);	/* 2^64+1 can be represented in 21 chars. */
			if (str) sprintf(str,"%d",item->valueint);
		}
		else
		{
			str=(char*)cJSON_malloc(64);	/* This is a nice tradeoff. */
			if (str)
			{
				if (fabs(floor(d)-d)<=DBL_EPSILON)			sprintf(str,"%.0f",d);
				else if (fabs(d)<1.0e-6 || fabs(d)>1.0e9)	sprintf(str,"%e",d);
				else										sprintf(str,"%f",d);
			}
		}
		return str;
	}

	/* Parse the input text into an unescaped cstring, and populate item. */
	static const unsigned char firstByteMark[7] = { 0x00, 0x00, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC };
	static const char *parse_string(cJSON *item,const char *str)
	{
		const char *ptr=str+1;char *ptr2;char *out;int len=0;unsigned uc;
		if (*str!='\"') {ep=str;return 0;}	/* not a string! */
	
		while (*ptr!='\"' && *ptr && ++len) if (*ptr++ == '\\') ptr++;	/* Skip escaped quotes. */
	
		out=(char*)cJSON_malloc(len+1);	/* This is how long we need for the string, roughly. */
		if (!out) return 0;
	
		ptr=str+1;ptr2=out;
		while (*ptr!='\"' && *ptr)
		{
			if (*ptr!='\\') *ptr2++=*ptr++;
			else
			{
				ptr++;
				switch (*ptr)
				{
					case 'b': *ptr2++='\b';	break;
					case 'f': *ptr2++='\f';	break;
					case 'n': *ptr2++='\n';	break;
					case 'r': *ptr2++='\r';	break;
					case 't': *ptr2++='\t';	break;
					case 'u':	 /* transcode utf16 to utf8. DOES NOT SUPPORT SURROGATE PAIRS CORRECTLY. */
						sscanf(ptr+1,"%4x",&uc);	/* get the unicode char. */
						len=3;if (uc<0x80) len=1;else if (uc<0x800) len=2;ptr2+=len;
					
						switch (len) {
							case 3: *--ptr2 =((uc | 0x80) & 0xBF); uc >>= 6;
							case 2: *--ptr2 =((uc | 0x80) & 0xBF); uc >>= 6;
							case 1: *--ptr2 =(uc | firstByteMark[len]);
						}
						ptr2+=len;ptr+=4;
						break;
					default:  *ptr2++=*ptr; break;
				}
				ptr++;
			}
		}
		*ptr2=0;
		if (*ptr=='\"') ptr++;
		item->valuestring=out;
		item->type=cJSON_String;
		return ptr;
	}

	/* Render the cstring provided to an escaped version that can be printed. */
	static char *print_string_ptr(const char *str)
	{
		const char *ptr;char *ptr2,*out;int len=0;unsigned char token;
	
		if (!str) return cJSON_strdup("");
		ptr=str;while ((token=*ptr) && ++len) {if (strchr("\"\\\b\f\n\r\t",token)) len++; else if (token<32) len+=5;ptr++;}
	
		out=(char*)cJSON_malloc(len+3);
		if (!out) return 0;

		ptr2=out;ptr=str;
		*ptr2++='\"';
		while (*ptr)
		{
			if ((unsigned char)*ptr>31 && *ptr!='\"' && *ptr!='\\') *ptr2++=*ptr++;
			else
			{
				*ptr2++='\\';
				switch (token=*ptr++)
				{
					case '\\':	*ptr2++='\\';	break;
					case '\"':	*ptr2++='\"';	break;
					case '\b':	*ptr2++='b';	break;
					case '\f':	*ptr2++='f';	break;
					case '\n':	*ptr2++='n';	break;
					case '\r':	*ptr2++='r';	break;
					case '\t':	*ptr2++='t';	break;
					default: sprintf(ptr2,"u%04x",token);ptr2+=5;	break;	/* escape and print */
				}
			}
		}
		*ptr2++='\"';*ptr2++=0;
		return out;
	}
	/* Invote print_string_ptr (which is useful) on an item. */
	static char *print_string(cJSON *item)	{return print_string_ptr(item->valuestring);}

	/* Predeclare these prototypes. */
	static const char *parse_value(cJSON *item,const char *value);
	static char *print_value(cJSON *item,int depth,int fmt);
	static const char *parse_array(cJSON *item,const char *value);
	static char *print_array(cJSON *item,int depth,int fmt);
	static const char *parse_object(cJSON *item,const char *value);
	static char *print_object(cJSON *item,int depth,int fmt);

	/* Utility to jump whitespace and cr/lf */
	static const char *skip(const char *in) {while (in && *in && (unsigned char)*in<=32) in++; return in;}

	/* Parse an object - create a new root, and populate. */
	cJSON *cJSON_Parse(const char *value)
	{
		cJSON *c=cJSON_New_Item();
		ep=0;
		if (!c) return 0;       /* memory fail */

		if (!parse_value(c,skip(value))) {cJSON_Delete(c);return 0;}
		return c;
	}

	/* Render a cJSON item/entity/structure to text. */
	char *cJSON_Print(cJSON *item)				{return print_value(item,0,1);}
	char *cJSON_PrintUnformatted(cJSON *item)	{return print_value(item,0,0);}

	/* Parser core - when encountering text, process appropriately. */
	static const char *parse_value(cJSON *item,const char *value)
	{
		if (!value)						return 0;	/* Fail on null. */
		if (!strncmp(value,"null",4))	{ item->type=cJSON_NULL;  return value+4; }
		if (!strncmp(value,"false",5))	{ item->type=cJSON_False; return value+5; }
		if (!strncmp(value,"true",4))	{ item->type=cJSON_True; item->valueint=1;	return value+4; }
		if (*value=='\"')				{ return parse_string(item,value); }
		if (*value=='-' || (*value>='0' && *value<='9'))	{ return parse_number(item,value); }
		if (*value=='[')				{ return parse_array(item,value); }
		if (*value=='{')				{ return parse_object(item,value); }

		ep=value;return 0;	/* failure. */
	}

	/* Render a value to text. */
	static char *print_value(cJSON *item,int depth,int fmt)
	{
		char *out=0;
		if (!item) return 0;
		switch ((item->type)&255)
		{
			case cJSON_NULL:	out=cJSON_strdup("null");	break;
			case cJSON_False:	out=cJSON_strdup("false");break;
			case cJSON_True:	out=cJSON_strdup("true"); break;
			case cJSON_Number:	out=print_number(item);break;
			case cJSON_String:	out=print_string(item);break;
			case cJSON_Array:	out=print_array(item,depth,fmt);break;
			case cJSON_Object:	out=print_object(item,depth,fmt);break;
		}
		return out;
	}

	/* Build an array from input text. */
	static const char *parse_array(cJSON *item,const char *value)
	{
		cJSON *child;
		if (*value!='[')	{ep=value;return 0;}	/* not an array! */

		item->type=cJSON_Array;
		value=skip(value+1);
		if (*value==']') return value+1;	/* empty array. */

		item->child=child=cJSON_New_Item();
		if (!item->child) return 0;		 /* memory fail */
		value=skip(parse_value(child,skip(value)));	/* skip any spacing, get the value. */
		if (!value) return 0;

		while (*value==',')
		{
			cJSON *new_item;
			if (!(new_item=cJSON_New_Item())) return 0; 	/* memory fail */
			child->next=new_item;new_item->prev=child;child=new_item;
			value=skip(parse_value(child,skip(value+1)));
			if (!value) return 0;	/* memory fail */
		}

		if (*value==']') return value+1;	/* end of array */
		ep=value;return 0;	/* malformed. */
	}

	/* Render an array to text */
	static char *print_array(cJSON *item,int depth,int fmt)
	{
		char **entries;
		char *out=0,*ptr,*ret;int len=5;
		cJSON *child=item->child;
		int numentries=0,i=0,fail=0;
	
		/* How many entries in the array? */
		while (child) numentries++,child=child->next;









		/* Allocate an array to hold the values for each */
		entries=(char**)cJSON_malloc(numentries*sizeof(char*));
		if (!entries) return 0;
		memset(entries,0,numentries*sizeof(char*));
		/* Retrieve all the results: */
		child=item->child;
		while (child && !fail)
		{
			ret=print_value(child,depth+1,fmt);
			entries[i++]=ret;
			if (ret) len+=strlen(ret)+2+(fmt?1:0); else fail=1;
			child=child->next;
		}
	
		/* If we didn't fail, try to malloc the output string */
		if (!fail) out=(char*)cJSON_malloc(len);
		/* If that fails, we fail. */
		if (!out) fail=1;

		/* Handle failure. */
		if (fail)
		{
			for (i=0;i<numentries;i++) if (entries[i]) cJSON_free(entries[i]);
			cJSON_free(entries);
			return 0;
		}
	
		/* Compose the output array. */
		*out='[';
		ptr=out+1;*ptr=0;
		for (i=0;i<numentries;i++)
		{
			strcpy(ptr,entries[i]);ptr+=strlen(entries[i]);
			if (i!=numentries-1) {*ptr++=',';if(fmt)*ptr++=' ';*ptr=0;}
			cJSON_free(entries[i]);
		}
		cJSON_free(entries);
		*ptr++=']';*ptr++=0;
		return out;	
	}

	/* Build an object from the text. */
	static const char *parse_object(cJSON *item,const char *value)
	{
		cJSON *child;
		if (*value!='{')	{ep=value;return 0;}	/* not an object! */
	
		item->type=cJSON_Object;
		value=skip(value+1);
		if (*value=='}') return value+1;	/* empty array. */
	
		item->child=child=cJSON_New_Item();
		if (!item->child) return 0;
		value=skip(parse_string(child,skip(value)));
		if (!value) return 0;
		child->string=child->valuestring;child->valuestring=0;
		if (*value!=':') {ep=value;return 0;}	/* fail! */
		value=skip(parse_value(child,skip(value+1)));	/* skip any spacing, get the value. */
		if (!value) return 0;
	
		while (*value==',')
		{
			cJSON *new_item;
			if (!(new_item=cJSON_New_Item()))	return 0; /* memory fail */
			child->next=new_item;new_item->prev=child;child=new_item;
			value=skip(parse_string(child,skip(value+1)));
			if (!value) return 0;
			child->string=child->valuestring;child->valuestring=0;
			if (*value!=':') {ep=value;return 0;}	/* fail! */
			value=skip(parse_value(child,skip(value+1)));	/* skip any spacing, get the value. */
			if (!value) return 0;
		}
	
		if (*value=='}') return value+1;	/* end of array */
		ep=value;return 0;	/* malformed. */
	}

	/* Render an object to text. */
	static char *print_object(cJSON *item,int depth,int fmt)
	{
		char **entries=0,**names=0;
		char *out=0,*ptr,*ret,*str;int len=7,i=0,j;
		cJSON *child=item->child;
		int numentries=0,fail=0;
		/* Count the number of entries. */
		while (child) numentries++,child=child->next;
		/* Allocate space for the names and the objects */
		entries=(char**)cJSON_malloc(numentries*sizeof(char*));
		if (!entries) return 0;
		names=(char**)cJSON_malloc(numentries*sizeof(char*));
		if (!names) {cJSON_free(entries);return 0;}
		memset(entries,0,sizeof(char*)*numentries);
		memset(names,0,sizeof(char*)*numentries);

		/* Collect all the results into our arrays: */
		child=item->child;depth++;if (fmt) len+=depth;
		while (child)
		{
			names[i]=str=print_string_ptr(child->string);
			entries[i++]=ret=print_value(child,depth,fmt);
			if (str && ret) len+=strlen(ret)+strlen(str)+2+(fmt?2+depth:0); else fail=1;
			child=child->next;
		}
	
		/* Try to allocate the output string */
		if (!fail) out=(char*)cJSON_malloc(len);
		if (!out) fail=1;

		/* Handle failure */
		if (fail)
		{
			for (i=0;i<numentries;i++) {if (names[i]) cJSON_free(names[i]);if (entries[i]) cJSON_free(entries[i]);}
			cJSON_free(names);cJSON_free(entries);
			return 0;
		}
	
		/* Compose the output: */
		*out='{';ptr=out+1;if (fmt)*ptr++='\n';*ptr=0;
		for (i=0;i<numentries;i++)
		{
			if (fmt) for (j=0;j<depth;j++) *ptr++='\t';
			strcpy(ptr,names[i]);ptr+=strlen(names[i]);
			*ptr++=':';if (fmt) *ptr++='\t';
			strcpy(ptr,entries[i]);ptr+=strlen(entries[i]);
			if (i!=numentries-1) *ptr++=',';
			if (fmt) *ptr++='\n';*ptr=0;
			cJSON_free(names[i]);cJSON_free(entries[i]);
		}
	
		cJSON_free(names);cJSON_free(entries);
		if (fmt) for (i=0;i<depth-1;i++) *ptr++='\t';
		*ptr++='}';*ptr++=0;
		return out;	
	}

	/* Get Array size/item / object item. */
	int    cJSON_GetArraySize(cJSON *array)							{cJSON *c=array->child;int i=0;while(c)i++,c=c->next;return i;}
	cJSON *cJSON_GetArrayItem(cJSON *array,int item)				{cJSON *c=array->child;  while (c && item>0) item--,c=c->next; return c;}
	cJSON *cJSON_GetObjectItem(cJSON *object,const char *string)	{cJSON *c=object->child; while (c && cJSON_strcasecmp(c->string,string)) c=c->next; return c;}

	/* Utility for array list handling. */
	static void suffix_object(cJSON *prev,cJSON *item) {prev->next=item;item->prev=prev;}
	/* Utility for handling references. */
	static cJSON *create_reference(cJSON *item) {cJSON *ref=cJSON_New_Item();if (!ref) return 0;memcpy(ref,item,sizeof(cJSON));ref->string=0;ref->type|=cJSON_IsReference;ref->next=ref->prev=0;return ref;}

	/* Add item to array/object. */
	void   cJSON_AddItemToArray(cJSON *array, cJSON *item)						{cJSON *c=array->child;if (!item) return; if (!c) {array->child=item;} else {while (c && c->next) c=c->next; suffix_object(c,item);}}
	void   cJSON_AddItemToObject(cJSON *object,const char *string,cJSON *item)	{if (!item) return; if (item->string) cJSON_free(item->string);item->string=cJSON_strdup(string);cJSON_AddItemToArray(object,item);}
	void	cJSON_AddItemReferenceToArray(cJSON *array, cJSON *item)						{cJSON_AddItemToArray(array,create_reference(item));}
	void	cJSON_AddItemReferenceToObject(cJSON *object,const char *string,cJSON *item)	{cJSON_AddItemToObject(object,string,create_reference(item));}

	cJSON *cJSON_DetachItemFromArray(cJSON *array,int which)			{cJSON *c=array->child;while (c && which>0) c=c->next,which--;if (!c) return 0;
		if (c->prev) c->prev->next=c->next;if (c->next) c->next->prev=c->prev;if (c==array->child) array->child=c->next;c->prev=c->next=0;return c;}
	void   cJSON_DeleteItemFromArray(cJSON *array,int which)			{cJSON_Delete(cJSON_DetachItemFromArray(array,which));}
	cJSON *cJSON_DetachItemFromObject(cJSON *object,const char *string) {int i=0;cJSON *c=object->child;while (c && cJSON_strcasecmp(c->string,string)) i++,c=c->next;if (c) return cJSON_DetachItemFromArray(object,i);return 0;}
	void   cJSON_DeleteItemFromObject(cJSON *object,const char *string) {cJSON_Delete(cJSON_DetachItemFromObject(object,string));}

	/* Replace array/object items with new ones. */
	void   cJSON_ReplaceItemInArray(cJSON *array,int which,cJSON *newitem)		{cJSON *c=array->child;while (c && which>0) c=c->next,which--;if (!c) return;
		newitem->next=c->next;newitem->prev=c->prev;if (newitem->next) newitem->next->prev=newitem;
		if (c==array->child) array->child=newitem; else newitem->prev->next=newitem;c->next=c->prev=0;cJSON_Delete(c);}
	void   cJSON_ReplaceItemInObject(cJSON *object,const char *string,cJSON *newitem){int i=0;cJSON *c=object->child;while(c && cJSON_strcasecmp(c->string,string))i++,c=c->next;if(c){newitem->string=cJSON_strdup(string);cJSON_ReplaceItemInArray(object,i,newitem);}}

	/* Create basic types: */
	cJSON *cJSON_CreateNull()						{cJSON *item=cJSON_New_Item();if(item)item->type=cJSON_NULL;return item;}
	cJSON *cJSON_CreateTrue()						{cJSON *item=cJSON_New_Item();if(item)item->type=cJSON_True;return item;}
	cJSON *cJSON_CreateFalse()						{cJSON *item=cJSON_New_Item();if(item)item->type=cJSON_False;return item;}
	cJSON *cJSON_CreateBool(int b)					{cJSON *item=cJSON_New_Item();if(item)item->type=b?cJSON_True:cJSON_False;return item;}
	cJSON *cJSON_CreateNumber(double num)			{cJSON *item=cJSON_New_Item();if(item){item->type=cJSON_Number;item->valuedouble=num;item->valueint=(int)num;}return item;}
	cJSON *cJSON_CreateString(const char *string)	{cJSON *item=cJSON_New_Item();if(item){item->type=cJSON_String;item->valuestring=cJSON_strdup(string);}return item;}
	cJSON *cJSON_CreateArray()						{cJSON *item=cJSON_New_Item();if(item)item->type=cJSON_Array;return item;}
	cJSON *cJSON_CreateObject()						{cJSON *item=cJSON_New_Item();if(item)item->type=cJSON_Object;return item;}

	/* Create Arrays: */
	cJSON *cJSON_CreateIntArray(int *numbers,int count)				{int i;cJSON *n=0,*p=0,*a=cJSON_CreateArray();for(i=0;a && i<count;i++){n=cJSON_CreateNumber(numbers[i]);if(!i)a->child=n;else suffix_object(p,n);p=n;}return a;}
	cJSON *cJSON_CreateFloatArray(float *numbers,int count)			{int i;cJSON *n=0,*p=0,*a=cJSON_CreateArray();for(i=0;a && i<count;i++){n=cJSON_CreateNumber(numbers[i]);if(!i)a->child=n;else suffix_object(p,n);p=n;}return a;}
	cJSON *cJSON_CreateDoubleArray(double *numbers,int count)		{int i;cJSON *n=0,*p=0,*a=cJSON_CreateArray();for(i=0;a && i<count;i++){n=cJSON_CreateNumber(numbers[i]);if(!i)a->child=n;else suffix_object(p,n);p=n;}return a;}
	cJSON *cJSON_CreateStringArray(const char **strings,int count)	{int i;cJSON *n=0,*p=0,*a=cJSON_CreateArray();for(i=0;a && i<count;i++){n=cJSON_CreateString(strings[i]);if(!i)a->child=n;else suffix_object(p,n);p=n;}return a;}
}


namespace glf
{
	//--------------------------------------------------------------------------
	namespace io
	{
		namespace
		{
			//------------------------------------------------------------------
			typedef std::map<std::string,glf::Texture2D*> TextureDB;
			//------------------------------------------------------------------
			std::string ValidFilename(	const std::string& _inFolder, 
										const std::string& _inFile, 
										const std::string& _default)
			{
				return _inFile.compare("")==0?_default:(_inFolder+_inFile);
			}
			//------------------------------------------------------------------
			bool FindTexture(	const std::string& _filename, 
								const TextureDB& _textureDB,
								Texture2D*& _texture)
			{
				TextureDB::const_iterator it = _textureDB.find(_filename);
				if(it != _textureDB.end())
				{
					_texture = it->second;
					return true;
				}
				else
				{
					_texture = NULL;
					return false;
				}
				return false;
			}
			//------------------------------------------------------------------
			Texture2D* GetDiffuseTex(	const std::string& _folder,
										const std::string& _filename, 
										TextureDB& _textureDB,
										ResourceManager& _resourceManager)
			{
				std::string filename = ValidFilename(_folder,_filename,"defaultdiffuse");
				Texture2D* texture   = NULL;
				if(!FindTexture(filename,_textureDB,texture))
				{
					texture = _resourceManager.CreateTexture2D();
					LoadTexture(filename,*texture,true);

					texture->SetFiltering(GL_LINEAR_MIPMAP_LINEAR,GL_LINEAR);
					texture->SetAnisotropy(MAX_ANISOSTROPY);
					glBindTexture(texture->target,texture->id);
					glGenerateMipmap(GL_TEXTURE_2D);
					
					_textureDB[filename] = texture;
				}
				return texture;
			}
			//------------------------------------------------------------------
			Texture2D* GetNormalTex(	const std::string& _folder,
										const std::string& _filename,
										TextureDB& _textureDB,
										ResourceManager& _resourceManager)
			{
				std::string filename = ValidFilename(_folder,_filename,"defaultnormal");
				Texture2D* texture   = NULL;
				if(!FindTexture(filename,_textureDB,texture))
				{
					texture = _resourceManager.CreateTexture2D();
					LoadTexture(filename,*texture,false);

					texture->SetFiltering(GL_LINEAR_MIPMAP_LINEAR,GL_LINEAR);
					texture->SetAnisotropy(MAX_ANISOSTROPY);
					glBindTexture(texture->target,texture->id);
					glGenerateMipmap(GL_TEXTURE_2D);
					
					_textureDB[filename] = texture;
				}
				return texture;
			}
			//------------------------------------------------------------------
			void InitializeDB(	TextureDB& _textureDB,
								ResourceManager& _resourceManager)
			{
				// Create default textures
				Texture2D* diffuseTex = _resourceManager.CreateTexture2D();
				Texture2D* normalTex  = _resourceManager.CreateTexture2D();

				unsigned char defaultColor[] = {255,255,255};
				diffuseTex->Allocate(GL_SRGB8_ALPHA8,1,1);
				diffuseTex->Fill(GL_RGB,GL_UNSIGNED_BYTE,defaultColor);
				diffuseTex->SetFiltering(GL_LINEAR,GL_LINEAR);

				unsigned char defaultNormal[] = {128,128,255};
				normalTex->Allocate(GL_RGBA8,1,1);
				normalTex->Fill(GL_RGB,GL_UNSIGNED_BYTE,defaultNormal);
				normalTex->SetFiltering(GL_LINEAR,GL_LINEAR);

				_textureDB["defaultdiffuse"] = diffuseTex;
				_textureDB["defaultnormal"]  = normalTex;
			}
		}
		//----------------------------------------------------------------------
		void LoadScene(		const std::string& _folder,
							const std::string& _filename,
							const glm::mat4& _transform,
							ResourceManager& _resourceManager,
							SceneManager& _scene,
							HelperManager& _helpers,
							bool _verbose)
		{
			// Load objects
			ModelOBJ loader;
			assert(loader.import((_folder+_filename).c_str(), true));

			int nObjects = loader.getNumberOfMeshes();
			if(_verbose)
			{
				glf::Info("Folder          : %s",_folder.c_str());
				glf::Info("Filename        : %s",_filename.c_str());
				glf::Info("nObjects        : %d",nObjects);
				glf::Info("hasNormals      : %d",loader.hasNormals());
				glf::Info("hasPositions    : %d",loader.hasPositions());
				glf::Info("hasTangents     : %d",loader.hasTangents());
				glf::Info("hasTextureCoords: %d",loader.hasTextureCoords());
				glf::Info("nVertices       : %d",loader.getNumberOfVertices());
				glf::Info("nIndices        : %d",loader.getNumberOfIndices());
				glf::Info("nTriangles      : %d",loader.getNumberOfTriangles());
			}
			assert(loader.hasNormals());
			assert(loader.hasTangents());
			assert(loader.hasTextureCoords());

			// Create VBO
			glf::VertexBuffer3F* vb = _resourceManager.CreateVBO3F();
			glf::VertexBuffer3F* nb = _resourceManager.CreateVBO3F();
			glf::VertexBuffer4F* tb = _resourceManager.CreateVBO4F();
			glf::VertexBuffer2F* ub = _resourceManager.CreateVBO2F();

			int nVertices = loader.getNumberOfVertices();
			vb->Allocate(nVertices,GL_STATIC_DRAW);
			nb->Allocate(nVertices,GL_STATIC_DRAW);
			tb->Allocate(nVertices,GL_STATIC_DRAW);
			ub->Allocate(nVertices,GL_STATIC_DRAW);

			glm::mat3 rotTransform = glm::mat3(_transform);
			const ModelOBJ::Vertex* vSource = loader.getVertexBuffer();
			glm::vec3* vptr = vb->Lock();
			glm::vec3* nptr = nb->Lock();
			glm::vec4* tptr = tb->Lock();
			glm::vec2* uptr = ub->Lock();
			for(int i=0;i<nVertices;++i)
			{
				vptr[i].x = vSource[i].position[0];
				vptr[i].y = vSource[i].position[1];
				vptr[i].z = vSource[i].position[2];
				vptr[i]   = glm::vec3(_transform * glm::vec4(vptr[i],1.f));

				nptr[i].x = vSource[i].normal[0];
				nptr[i].y = vSource[i].normal[1];
				nptr[i].z = vSource[i].normal[2];
				nptr[i]   = rotTransform * nptr[i];

				tptr[i].x = vSource[i].tangent[0];
				tptr[i].y = vSource[i].tangent[1];
				tptr[i].z = vSource[i].tangent[2];
				tptr[i].w = 0; 						// For removing translation
				tptr[i]   = _transform * tptr[i];

				glm::vec3 bitangent;
				bitangent.x = vSource[i].bitangent[0];
				bitangent.y = vSource[i].bitangent[1];
				bitangent.z = vSource[i].bitangent[2];
				bitangent   = rotTransform * bitangent;

				uptr[i].x = vSource[i].texCoord[0];
				uptr[i].y = vSource[i].texCoord[1];

				// Compute the referential's handedness and store its sign 
				// into w component of the tangent vector
				tptr[i].w = glm::dot(bitangent,glm::normalize(glm::cross(nptr[i],glm::vec3(tptr[i]))));
				//glf::Info("Sign : %f",tptr[i].w);
			}
			ub->Unlock();
			tb->Unlock();
			nb->Unlock();
			vb->Unlock();

			// Create IBO
			glf::IndexBuffer* ib = _resourceManager.CreateIBO();
			int nIndices = loader.getNumberOfIndices();
			ib->Allocate(nIndices);
			const int* iSource = loader.getIndexBuffer();
			unsigned int* iptr = ib->Lock();
			for(int i=0;i<nIndices;++i)
				iptr[i] = iSource[i];
			ib->Unlock();

			// Create VAOs
			glf::VertexArray* regularVAO = _resourceManager.CreateVAO();
			regularVAO->Add(*vb,semantic::Position, 3,GL_FLOAT,false,0);
			regularVAO->Add(*nb,semantic::Normal,   3,GL_FLOAT,false,0);
			regularVAO->Add(*tb,semantic::Tangent,  4,GL_FLOAT,false,0);
			regularVAO->Add(*ub,semantic::TexCoord, 2,GL_FLOAT,false,0);

			glf::VertexArray* shadowVAO  = _resourceManager.CreateVAO();
			shadowVAO->Add(*vb,semantic::Position,  3,GL_FLOAT,false,0);

			// Create objets and load textures
			TextureDB textureDB;
			InitializeDB(textureDB,_resourceManager);
			for(int i=0;i<nObjects;++i)
			{
				const ModelOBJ::Mesh& mesh	= loader.getMesh(i);

				TextureDB::iterator it;

				// Load textures
				glf::Texture2D* diffuseTex = GetDiffuseTex(_folder,mesh.pMaterial->colorMapFilename,textureDB,_resourceManager);
				//glf::Texture2D* normalTex  = GetNormalTex(_folder,mesh.pMaterial->bumpMapFilename,textureDB,_resourceManager);
				glf::Texture2D* normalTex  = GetNormalTex(_folder,"",textureDB,_resourceManager);

				// Create and add regular mesh
				RegularMesh rmesh;
				rmesh.diffuseTex   = diffuseTex;
				rmesh.normalTex    = normalTex;
				rmesh.roughness    = mesh.pMaterial->shininess * 1000.f; // (Has to be specified as roughness into MTL file)
				rmesh.specularity  = 0.25 * (mesh.pMaterial->specular[0]+mesh.pMaterial->specular[1]+mesh.pMaterial->specular[2]+mesh.pMaterial->specular[3]);
				rmesh.indices      = ib;
				rmesh.startIndices = mesh.startIndex;
				rmesh.countIndices = mesh.triangleCount*3;
				rmesh.primitiveType= GL_TRIANGLES;
				rmesh.primitive    = regularVAO;
				_scene.regularMeshes.push_back(rmesh);

				// Create and add shadow mesh
				ShadowMesh smesh;
				smesh.indices      = ib;
				smesh.startIndices = mesh.startIndex;
				smesh.countIndices = mesh.triangleCount*3;
				smesh.primitiveType= GL_TRIANGLES;
				smesh.primitive    = shadowVAO;
				_scene.shadowMeshes.push_back(smesh);

				glm::mat4 identity(1);
				_scene.transformations.push_back(identity);

				BBox obound = ObjectBound(*vb,*ib,rmesh.startIndices,rmesh.countIndices);
				_scene.oBounds.push_back(obound);

				#if ADD_TBN_HELPERS
					_helpers.CreateTangentSpace(*vb,*nb,*tb,*ib,rmesh.startIndices,rmesh.countIndices,0.1f);
				#endif

				if(_verbose)
				{
					glf::Info("----------------------------------------------");
					glf::Info("MeshID       : %d",i);
					glf::Info("startIndex   : %d",mesh.startIndex);
					glf::Info("triangleCount: %d",mesh.triangleCount);

					glf::Info("Ambient   : %f,%f,%f,%f",
								mesh.pMaterial->ambient[0],
								mesh.pMaterial->ambient[1],
								mesh.pMaterial->ambient[2],
								mesh.pMaterial->ambient[3]);

					glf::Info("Diffuse   : %f,%f,%f,%f",
								mesh.pMaterial->diffuse[0],
								mesh.pMaterial->diffuse[1],
								mesh.pMaterial->diffuse[2],
								mesh.pMaterial->diffuse[3]);

					glf::Info("Specular  : %f,%f,%f,%f",
								mesh.pMaterial->specular[0],
								mesh.pMaterial->specular[1],
								mesh.pMaterial->specular[2],
								mesh.pMaterial->specular[3]);

					glf::Info("Shininess : %f",mesh.pMaterial->shininess);
					glf::Info("Alpha     : %f",mesh.pMaterial->alpha);
					glf::Info("Name      : %s",mesh.pMaterial->name.c_str());
					glf::Info("Color     : %s",mesh.pMaterial->colorMapFilename.c_str());
					glf::Info("Bump      : %s",mesh.pMaterial->bumpMapFilename.c_str());

					glf::Info("Bound     : (%f,%f,%f) (%f,%f,%f)",
											obound.pMin.x,
											obound.pMin.y,
											obound.pMin.z,
											obound.pMax.x,
											obound.pMax.y,
											obound.pMax.z);
				}
			}
			_scene.wBound = WorldBound(_scene);

			if(_verbose)
			{
				glf::Info("----------------------------------------------");
				glf::Info("World bound   : (%f,%f,%f) (%f,%f,%f)",
											_scene.wBound.pMin.x,
											_scene.wBound.pMin.y,
											_scene.wBound.pMin.z,
											_scene.wBound.pMax.x,
											_scene.wBound.pMax.y,
											_scene.wBound.pMax.z);
			}

			loader.destroy();
		}


		#if 0
		// JSON loading example
		{
			// Load file
			std::ifstream file(_filename.c_str());

			// Compute file size
			std::size_t dataSize;
			{
				long pos = file.tellg();
				file.seekg(0, std::ios_base::end);
				dataSize = file.tellg();
				file.seekg(pos, std::ios_base::beg);
			}

			// Load data
			char* fileContent = new char[dataSize];
			file.read(fileContent,sizeof(char)*dataSize);

			// Parse data
			cJSON *root   = cJSON_Parse(fileContent);
			Info("firstName : %s",cJSON_GetObjectItem(root,"firstName")->valuestring);

			delete[] fileContent;
		}
		#endif
	}
}
