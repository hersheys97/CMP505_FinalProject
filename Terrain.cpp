#include "pch.h"
#include "Terrain.h"	
#include <float.h>
using namespace DirectX;
using namespace DirectX::SimpleMath;

Terrain::Terrain()
{
	m_terrainGeneratedToggle = false;
}

Terrain::~Terrain() {}

bool Terrain::Initialize(ID3D11Device* device, int terrainWidth, int terrainHeight)
{
	int index;
	float height = 0.0;
	bool result;

	m_device = device;

	// Save the dimensions of the terrain.
	m_terrainWidth = terrainWidth;
	m_terrainHeight = terrainHeight;

	m_frequency = m_terrainWidth / 20;
	m_amplitude = 3.0;
	m_wavelength = 1;

	// Create the structure to hold the terrain data.
	m_heightMap = new HeightMapType[m_terrainWidth * m_terrainHeight];
	if (!m_heightMap) return false;

	float textureCoordinatesStep = 5.0f / m_terrainWidth;
	const float triangleSize = 1.0f;
	const float heightFactor = sqrt(3.0f) / 2.0f * triangleSize;

	for (int j = 0; j < m_terrainHeight; j++)
	{
		for (int i = 0; i < m_terrainWidth; i++)
		{
			index = (m_terrainHeight * j) + i;

			// Offset every other row by half a triangle width
			float xOffset = (j % 2) * (triangleSize * 0.5f);

			// X position: regular spacing with offset for odd rows
			float xPos = i * triangleSize + xOffset;

			// Z position: rows are spaced by heightFactor
			float zPos = j * heightFactor;

			m_heightMap[index].x = xPos;
			m_heightMap[index].y = height;
			m_heightMap[index].z = zPos;

			// Texture coordinates
			m_heightMap[index].u = (float)i / (m_terrainWidth - 1);
			m_heightMap[index].v = (float)j / (m_terrainHeight - 1);
		}
	}

	//even though we are generating a flat terrain, we still need to normalise it. 
	// Calculate the normals for the terrain data.
	result = CalculateNormals();
	if (!result) return false;

	// Initialize the vertex and index buffer that hold the geometry for the terrain.
	result = InitializeBuffers(device);
	if (!result)return false;

	return true;
}

void Terrain::Render(ID3D11DeviceContext* deviceContext)
{
	// Put the vertex and index buffers on the graphics pipeline to prepare them for drawing.
	RenderBuffers(deviceContext);
	deviceContext->DrawIndexed(m_indexCount, 0, 0);

	return;
}

bool Terrain::CalculateNormals()
{
	int i, j, index1, index2, index3, index4, index, count;
	float vertex1[3], vertex2[3], vertex3[3], vector1[3], vector2[3], sum[3], length;
	DirectX::SimpleMath::Vector3* normals;

	// We need 2 normals per quad (one for each triangle)
	int numFaces = (m_terrainHeight - 1) * (m_terrainWidth - 1) * 2;
	normals = new DirectX::SimpleMath::Vector3[numFaces];
	if (!normals) return false;

	// Calculate face normals for all triangles
	int faceIndex = 0;
	for (j = 0; j < (m_terrainHeight - 1); j++)
	{
		for (i = 0; i < (m_terrainWidth - 1); i++)
		{
			// Get indices for the quad corners
			index1 = (j * m_terrainHeight) + i;          // Bottom left
			index2 = (j * m_terrainHeight) + (i + 1);    // Bottom right
			index3 = ((j + 1) * m_terrainHeight) + i;    // Top left
			index4 = ((j + 1) * m_terrainHeight) + (i + 1); // Top right

			// First triangle (either bottom-left, top-left, bottom-right OR
			// bottom-left, top-right, bottom-right depending on alternating pattern)
			if ((i + j) % 2 == 0) {
				// Triangle 1: bottom-left, top-left, bottom-right
				vertex1[0] = m_heightMap[index1].x;
				vertex1[1] = m_heightMap[index1].y;
				vertex1[2] = m_heightMap[index1].z;

				vertex2[0] = m_heightMap[index3].x;
				vertex2[1] = m_heightMap[index3].y;
				vertex2[2] = m_heightMap[index3].z;

				vertex3[0] = m_heightMap[index2].x;
				vertex3[1] = m_heightMap[index2].y;
				vertex3[2] = m_heightMap[index2].z;
			}
			else {
				// Triangle 1: bottom-left, top-right, bottom-right
				vertex1[0] = m_heightMap[index1].x;
				vertex1[1] = m_heightMap[index1].y;
				vertex1[2] = m_heightMap[index1].z;

				vertex2[0] = m_heightMap[index4].x;
				vertex2[1] = m_heightMap[index4].y;
				vertex2[2] = m_heightMap[index4].z;

				vertex3[0] = m_heightMap[index2].x;
				vertex3[1] = m_heightMap[index2].y;
				vertex3[2] = m_heightMap[index2].z;
			}

			// Calculate vectors for the triangle
			vector1[0] = vertex1[0] - vertex2[0];
			vector1[1] = vertex1[1] - vertex2[1];
			vector1[2] = vertex1[2] - vertex2[2];

			vector2[0] = vertex2[0] - vertex3[0];
			vector2[1] = vertex2[1] - vertex3[1];
			vector2[2] = vertex2[2] - vertex3[2];

			// Calculate cross product for the face normal
			normals[faceIndex].x = (vector1[1] * vector2[2]) - (vector1[2] * vector2[1]);
			normals[faceIndex].y = (vector1[2] * vector2[0]) - (vector1[0] * vector2[2]);
			normals[faceIndex].z = (vector1[0] * vector2[1]) - (vector1[1] * vector2[0]);
			faceIndex++;

			// Second triangle (either top-left, top-right, bottom-right OR
			// top-left, bottom-left, top-right depending on alternating pattern)
			if ((i + j) % 2 == 0) {
				// Triangle 2: top-left, top-right, bottom-right
				vertex1[0] = m_heightMap[index3].x;
				vertex1[1] = m_heightMap[index3].y;
				vertex1[2] = m_heightMap[index3].z;

				vertex2[0] = m_heightMap[index4].x;
				vertex2[1] = m_heightMap[index4].y;
				vertex2[2] = m_heightMap[index4].z;

				vertex3[0] = m_heightMap[index2].x;
				vertex3[1] = m_heightMap[index2].y;
				vertex3[2] = m_heightMap[index2].z;
			}
			else {
				// Triangle 2: top-left, bottom-left, top-right
				vertex1[0] = m_heightMap[index3].x;
				vertex1[1] = m_heightMap[index3].y;
				vertex1[2] = m_heightMap[index3].z;

				vertex2[0] = m_heightMap[index1].x;
				vertex2[1] = m_heightMap[index1].y;
				vertex2[2] = m_heightMap[index1].z;

				vertex3[0] = m_heightMap[index4].x;
				vertex3[1] = m_heightMap[index4].y;
				vertex3[2] = m_heightMap[index4].z;
			}

			// Calculate vectors for the triangle
			vector1[0] = vertex1[0] - vertex2[0];
			vector1[1] = vertex1[1] - vertex2[1];
			vector1[2] = vertex1[2] - vertex2[2];

			vector2[0] = vertex2[0] - vertex3[0];
			vector2[1] = vertex2[1] - vertex3[1];
			vector2[2] = vertex2[2] - vertex3[2];

			// Calculate cross product for the face normal
			normals[faceIndex].x = (vector1[1] * vector2[2]) - (vector1[2] * vector2[1]);
			normals[faceIndex].y = (vector1[2] * vector2[0]) - (vector1[0] * vector2[2]);
			normals[faceIndex].z = (vector1[0] * vector2[1]) - (vector1[1] * vector2[0]);
			faceIndex++;
		}
	}

	// Calculate vertex normals by averaging face normals
	for (j = 0; j < m_terrainHeight; j++)
	{
		for (i = 0; i < m_terrainWidth; i++)
		{
			sum[0] = 0.0f;
			sum[1] = 0.0f;
			sum[2] = 0.0f;
			count = 0;

			// Check all possible faces that include this vertex
			// Left faces
			if (i > 0) {
				// Bottom-left face
				if (j > 0) {
					index = ((j - 1) * (m_terrainWidth - 1) * 2) + ((i - 1) * 2);
					sum[0] += normals[index].x;
					sum[1] += normals[index].y;
					sum[2] += normals[index].z;
					count++;
				}

				// Top-left face
				if (j < m_terrainHeight - 1) {
					index = (j * (m_terrainWidth - 1) * 2) + ((i - 1) * 2);
					sum[0] += normals[index].x;
					sum[1] += normals[index].y;
					sum[2] += normals[index].z;
					count++;
				}
			}

			// Right faces
			if (i < m_terrainWidth - 1) {
				// Bottom-right face
				if (j > 0) {
					index = ((j - 1) * (m_terrainWidth - 1) * 2) + (i * 2);
					sum[0] += normals[index].x;
					sum[1] += normals[index].y;
					sum[2] += normals[index].z;
					count++;
				}

				// Top-right face
				if (j < m_terrainHeight - 1) {
					index = (j * (m_terrainWidth - 1) * 2) + (i * 2);
					sum[0] += normals[index].x;
					sum[1] += normals[index].y;
					sum[2] += normals[index].z;
					count++;
				}
			}

			// Calculate average normal
			if (count > 0) {
				sum[0] /= count;
				sum[1] /= count;
				sum[2] /= count;
			}

			// Normalize the final normal
			length = sqrt((sum[0] * sum[0]) + (sum[1] * sum[1]) + (sum[2] * sum[2]));
			if (length > 0) {
				sum[0] /= length;
				sum[1] /= length;
				sum[2] /= length;
			}

			// Store the normal
			index = (j * m_terrainHeight) + i;
			m_heightMap[index].nx = sum[0];
			m_heightMap[index].ny = sum[1];
			m_heightMap[index].nz = sum[2];
		}
	}

	// Clean up
	delete[] normals;
	normals = 0;

	return true;
}

void Terrain::Shutdown()
{
	// Release the index buffer.
	if (m_indexBuffer)
	{
		m_indexBuffer->Release();
		m_indexBuffer = 0;
	}

	// Release the vertex buffer.
	if (m_vertexBuffer)
	{
		m_vertexBuffer->Release();
		m_vertexBuffer = 0;
	}

	return;
}

bool Terrain::InitializeBuffers(ID3D11Device* device)
{
	VertexType* vertices;
	unsigned long* indices;
	D3D11_BUFFER_DESC vertexBufferDesc, indexBufferDesc;
	D3D11_SUBRESOURCE_DATA vertexData, indexData;
	HRESULT result;
	int index, i, j;
	int index1, index2, index3, index4; //geometric indices. 

	// Calculate the number of vertices in the terrain mesh.
	m_vertexCount = (m_terrainWidth - 1) * (m_terrainHeight - 1) * 6;

	// Set the index count to the same as the vertex count.
	m_indexCount = m_vertexCount;

	// Create the vertex array.
	vertices = new VertexType[m_vertexCount];
	if (!vertices) return false;

	// Create the index array.
	indices = new unsigned long[m_indexCount];
	if (!indices) return false;

	// Initialize the index to the vertex buffer.
	index = 0;

	for (j = 0; j < (m_terrainHeight - 1); j++)
	{
		for (i = 0; i < (m_terrainWidth - 1); i++)
		{
			index1 = (m_terrainHeight * j) + i;          // Bottom left
			index2 = (m_terrainHeight * j) + (i + 1);    // Bottom right
			index3 = (m_terrainHeight * (j + 1)) + i;    // Top left
			index4 = (m_terrainHeight * (j + 1)) + (i + 1); // Top right

			// Always create two triangles per quad, but alternate the diagonal
			if ((i + j) % 2 == 0) {
				// Triangle 1: bottom-left, top-left, bottom-right
				vertices[index].position = Vector3(m_heightMap[index1].x, m_heightMap[index1].y, m_heightMap[index1].z);
				vertices[index].normal = Vector3(m_heightMap[index1].nx, m_heightMap[index1].ny, m_heightMap[index1].nz);
				vertices[index].texture = Vector2(m_heightMap[index1].u, m_heightMap[index1].v);
				indices[index] = index;
				index++;

				vertices[index].position = Vector3(m_heightMap[index3].x, m_heightMap[index3].y, m_heightMap[index3].z);
				vertices[index].normal = Vector3(m_heightMap[index3].nx, m_heightMap[index3].ny, m_heightMap[index3].nz);
				vertices[index].texture = Vector2(m_heightMap[index3].u, m_heightMap[index3].v);
				indices[index] = index;
				index++;

				vertices[index].position = Vector3(m_heightMap[index2].x, m_heightMap[index2].y, m_heightMap[index2].z);
				vertices[index].normal = Vector3(m_heightMap[index2].nx, m_heightMap[index2].ny, m_heightMap[index2].nz);
				vertices[index].texture = Vector2(m_heightMap[index2].u, m_heightMap[index2].v);
				indices[index] = index;
				index++;

				// Triangle 2: top-left, top-right, bottom-right
				vertices[index].position = Vector3(m_heightMap[index3].x, m_heightMap[index3].y, m_heightMap[index3].z);
				vertices[index].normal = Vector3(m_heightMap[index3].nx, m_heightMap[index3].ny, m_heightMap[index3].nz);
				vertices[index].texture = Vector2(m_heightMap[index3].u, m_heightMap[index3].v);
				indices[index] = index;
				index++;

				vertices[index].position = Vector3(m_heightMap[index4].x, m_heightMap[index4].y, m_heightMap[index4].z);
				vertices[index].normal = Vector3(m_heightMap[index4].nx, m_heightMap[index4].ny, m_heightMap[index4].nz);
				vertices[index].texture = Vector2(m_heightMap[index4].u, m_heightMap[index4].v);
				indices[index] = index;
				index++;

				vertices[index].position = Vector3(m_heightMap[index2].x, m_heightMap[index2].y, m_heightMap[index2].z);
				vertices[index].normal = Vector3(m_heightMap[index2].nx, m_heightMap[index2].ny, m_heightMap[index2].nz);
				vertices[index].texture = Vector2(m_heightMap[index2].u, m_heightMap[index2].v);
				indices[index] = index;
				index++;
			}
			else {
				// Triangle 1: bottom-left, top-right, bottom-right
				vertices[index].position = Vector3(m_heightMap[index1].x, m_heightMap[index1].y, m_heightMap[index1].z);
				vertices[index].normal = Vector3(m_heightMap[index1].nx, m_heightMap[index1].ny, m_heightMap[index1].nz);
				vertices[index].texture = Vector2(m_heightMap[index1].u, m_heightMap[index1].v);
				indices[index] = index;
				index++;

				vertices[index].position = Vector3(m_heightMap[index4].x, m_heightMap[index4].y, m_heightMap[index4].z);
				vertices[index].normal = Vector3(m_heightMap[index4].nx, m_heightMap[index4].ny, m_heightMap[index4].nz);
				vertices[index].texture = Vector2(m_heightMap[index4].u, m_heightMap[index4].v);
				indices[index] = index;
				index++;

				vertices[index].position = Vector3(m_heightMap[index2].x, m_heightMap[index2].y, m_heightMap[index2].z);
				vertices[index].normal = Vector3(m_heightMap[index2].nx, m_heightMap[index2].ny, m_heightMap[index2].nz);
				vertices[index].texture = Vector2(m_heightMap[index2].u, m_heightMap[index2].v);
				indices[index] = index;
				index++;

				// Triangle 2: bottom-left, top-left, top-right
				vertices[index].position = Vector3(m_heightMap[index1].x, m_heightMap[index1].y, m_heightMap[index1].z);
				vertices[index].normal = Vector3(m_heightMap[index1].nx, m_heightMap[index1].ny, m_heightMap[index1].nz);
				vertices[index].texture = Vector2(m_heightMap[index1].u, m_heightMap[index1].v);
				indices[index] = index;
				index++;

				vertices[index].position = Vector3(m_heightMap[index3].x, m_heightMap[index3].y, m_heightMap[index3].z);
				vertices[index].normal = Vector3(m_heightMap[index3].nx, m_heightMap[index3].ny, m_heightMap[index3].nz);
				vertices[index].texture = Vector2(m_heightMap[index3].u, m_heightMap[index3].v);
				indices[index] = index;
				index++;

				vertices[index].position = Vector3(m_heightMap[index4].x, m_heightMap[index4].y, m_heightMap[index4].z);
				vertices[index].normal = Vector3(m_heightMap[index4].nx, m_heightMap[index4].ny, m_heightMap[index4].nz);
				vertices[index].texture = Vector2(m_heightMap[index4].u, m_heightMap[index4].v);
				indices[index] = index;
				index++;
			}
		}
	}

	// Set up the description of the static vertex buffer.
	vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	vertexBufferDesc.ByteWidth = sizeof(VertexType) * m_vertexCount;
	vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertexBufferDesc.CPUAccessFlags = 0;
	vertexBufferDesc.MiscFlags = 0;
	vertexBufferDesc.StructureByteStride = 0;

	// Give the subresource structure a pointer to the vertex data.
	vertexData.pSysMem = vertices;
	vertexData.SysMemPitch = 0;
	vertexData.SysMemSlicePitch = 0;

	// Now create the vertex buffer.
	result = device->CreateBuffer(&vertexBufferDesc, &vertexData, &m_vertexBuffer);
	if (FAILED(result)) return false;

	// Set up the description of the static index buffer.
	indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	indexBufferDesc.ByteWidth = sizeof(unsigned long) * m_indexCount;
	indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	indexBufferDesc.CPUAccessFlags = 0;
	indexBufferDesc.MiscFlags = 0;
	indexBufferDesc.StructureByteStride = 0;

	// Give the subresource structure a pointer to the index data.
	indexData.pSysMem = indices;
	indexData.SysMemPitch = 0;
	indexData.SysMemSlicePitch = 0;

	// Create the index buffer.
	result = device->CreateBuffer(&indexBufferDesc, &indexData, &m_indexBuffer);
	if (FAILED(result)) return false;

	// Release the arrays now that the vertex and index buffers have been created and loaded.
	delete[] vertices;
	vertices = 0;

	delete[] indices;
	indices = 0;

	return true;
}

void Terrain::RenderBuffers(ID3D11DeviceContext* deviceContext)
{
	unsigned int stride;
	unsigned int offset;

	// Set vertex buffer stride and offset.
	stride = sizeof(VertexType);
	offset = 0;

	// Set the vertex buffer to active in the input assembler so it can be rendered.
	deviceContext->IASetVertexBuffers(0, 1, &m_vertexBuffer, &stride, &offset);

	// Set the index buffer to active in the input assembler so it can be rendered.
	deviceContext->IASetIndexBuffer(m_indexBuffer, DXGI_FORMAT_R32_UINT, 0);

	// Set the type of primitive that should be rendered from this vertex buffer, in this case triangles.
	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	return;
}

bool Terrain::GenerateHeightMap(ID3D11Device* device)
{
	bool result;

	m_frequency = (6.283f / m_terrainWidth) / m_wavelength;

	const float triangleSize = 1.0f;
	const float heightFactor = sqrt(3.0f) / 2.0f;

	for (int j = 0; j < m_terrainHeight; j++)
	{
		for (int i = 0; i < m_terrainWidth; i++)
		{
			int index = (m_terrainHeight * j) + i;

			// Maintain the same vertex spacing as in Initialize()
			float xPos = (j % 2 == 0) ? i * triangleSize : (i + 0.5f) * triangleSize;
			float zPos = j * heightFactor;

			m_heightMap[index].x = xPos;
			m_heightMap[index].y = (sin((float)i * m_frequency) * m_amplitude);
			m_heightMap[index].z = zPos;

			// Update texture coordinates if needed
			m_heightMap[index].u = (float)i / (m_terrainWidth - 1);
			m_heightMap[index].v = (float)j / (m_terrainHeight - 1);
		}
	}

	result = CalculateNormals();
	if (!result) return false;

	result = InitializeBuffers(device);
	if (!result) return false;

	return true;
}

bool Terrain::RayIntersect(const DirectX::SimpleMath::Vector3& rayOrigin,
	const DirectX::SimpleMath::Vector3& rayDirection,
	float& distance,
	DirectX::SimpleMath::Vector3& hitPoint)
{
	bool hit = false;
	distance = FLT_MAX;

	// Iterate through all terrain triangles
	for (int j = 0; j < (m_terrainHeight - 1); j++)
	{
		for (int i = 0; i < (m_terrainWidth - 1); i++)
		{
			// Get indices for the quad corners
			int index1 = (j * m_terrainHeight) + i;          // Bottom left
			int index2 = (j * m_terrainHeight) + (i + 1);    // Bottom right
			int index3 = ((j + 1) * m_terrainHeight) + i;    // Top left
			int index4 = ((j + 1) * m_terrainHeight) + (i + 1); // Top right

			Vector3 v0, v1, v2;
			float tempDist;

			// Check first triangle (alternating pattern)
			if ((i + j) % 2 == 0) {
				v0 = Vector3(m_heightMap[index1].x, m_heightMap[index1].y, m_heightMap[index1].z);
				v1 = Vector3(m_heightMap[index3].x, m_heightMap[index3].y, m_heightMap[index3].z);
				v2 = Vector3(m_heightMap[index2].x, m_heightMap[index2].y, m_heightMap[index2].z);
			}
			else {
				v0 = Vector3(m_heightMap[index1].x, m_heightMap[index1].y, m_heightMap[index1].z);
				v1 = Vector3(m_heightMap[index4].x, m_heightMap[index4].y, m_heightMap[index4].z);
				v2 = Vector3(m_heightMap[index2].x, m_heightMap[index2].y, m_heightMap[index2].z);
			}

			if (RayTriangleIntersect(rayOrigin, rayDirection, v0, v1, v2, tempDist))
			{
				if (tempDist < distance)
				{
					distance = tempDist;
					hitPoint = rayOrigin + rayDirection * distance;
					hit = true;
				}
			}

			// Check second triangle (alternating pattern)
			if ((i + j) % 2 == 0) {
				v0 = Vector3(m_heightMap[index3].x, m_heightMap[index3].y, m_heightMap[index3].z);
				v1 = Vector3(m_heightMap[index4].x, m_heightMap[index4].y, m_heightMap[index4].z);
				v2 = Vector3(m_heightMap[index2].x, m_heightMap[index2].y, m_heightMap[index2].z);
			}
			else {
				v0 = Vector3(m_heightMap[index3].x, m_heightMap[index3].y, m_heightMap[index3].z);
				v1 = Vector3(m_heightMap[index1].x, m_heightMap[index1].y, m_heightMap[index1].z);
				v2 = Vector3(m_heightMap[index4].x, m_heightMap[index4].y, m_heightMap[index4].z);
			}

			if (RayTriangleIntersect(rayOrigin, rayDirection, v0, v1, v2, tempDist))
			{
				if (tempDist < distance)
				{
					distance = tempDist;
					hitPoint = rayOrigin + rayDirection * distance;
					hit = true;
				}
			}
		}
	}

	// Call HandleCollision when a hit occurs
	if (hit) HandleCollision(m_device, hitPoint); // Pass your D3D11 device pointer instead of nullptr

	return hit;
}

bool Terrain::RayTriangleIntersect(const Vector3& rayOrigin,
	const Vector3& rayDirection,
	const Vector3& v0,
	const Vector3& v1,
	const Vector3& v2,
	float& distance)
{
	// Möller-Trumbore algorithm implementation
	// Reference: 
	// - "Real-Time Rendering" by Tomas Akenine-Möller et al.
	// - "Fast, Minimum Storage Ray/Triangle Intersection" by Möller & Trumbore
	// - Scratchapixel tutorial: https://www.scratchapixel.com/lessons/3d-basic-rendering/ray-tracing-rendering-a-triangle/moller-trumbore-ray-triangle-intersection

	const float EPSILON = 0.0000001f;

	Vector3 edge1 = v1 - v0;
	Vector3 edge2 = v2 - v0;
	Vector3 h = rayDirection.Cross(edge2);

	float a = edge1.Dot(h);
	if (a > -EPSILON && a < EPSILON)
		return false;    // Ray parallel to triangle

	float f = 1.0f / a;
	Vector3 s = rayOrigin - v0;
	float u = f * s.Dot(h);

	if (u < 0.0f || u > 1.0f)
		return false;

	Vector3 q = s.Cross(edge1);
	float v = f * rayDirection.Dot(q);

	if (v < 0.0f || u + v > 1.0f)
		return false;

	// At this point we can compute t to find out where the intersection point is on the line
	float t = f * edge2.Dot(q);

	if (t > EPSILON) // Ray intersection
	{
		distance = t;
		return true;
	}

	return false;
}

void Terrain::HandleCollision(ID3D11Device* device, const DirectX::SimpleMath::Vector3& collisionPoint)
{
	// Check if we should generate heightmap (you can add additional conditions here)
	if (!m_terrainGeneratedToggle && device)
	{
		// Generate the heightmap
		GenerateHeightMap(device);
		m_terrainGeneratedToggle = true;

		// Optionally modify terrain at collision point
		// You could add code here to create a crater or raise terrain at the hit point
		// For example:
		/*
		for (int j = 0; j < m_terrainHeight; j++)
		{
			for (int i = 0; i < m_terrainWidth; i++)
			{
				int index = (m_terrainHeight * j) + i;
				Vector3 vertexPos(m_heightMap[index].x, m_heightMap[index].y, m_heightMap[index].z);
				float distance = Vector3::Distance(vertexPos, collisionPoint);

				if (distance < 5.0f) // Affect vertices within 5 units of collision
				{
					// Create a crater effect
					m_heightMap[index].y -= (5.0f - distance) * 0.2f;
				}
			}
		}

		// Recalculate normals and update buffers
		CalculateNormals();
		InitializeBuffers(device);
		*/
	}
}

bool Terrain::GetHeightAtPosition(float x, float z, float& height, Vector3& normal)
{
	// Check if we have valid terrain data
	if (!m_heightMap || m_terrainWidth <= 0 || m_terrainHeight <= 0)
		return false;

	// Convert world position to terrain grid coordinates
	float terrainScale = 20.0f; // Adjust this based on your terrain scale
	int i = static_cast<int>((x * terrainScale) + (m_terrainWidth / 2.0f));
	int j = static_cast<int>((z * terrainScale) + (m_terrainHeight / 2.0f));

	// Clamp to terrain bounds
	i = std::max(0, std::min(i, m_terrainWidth - 1));
	j = std::max(0, std::min(j, m_terrainHeight - 1));

	int index = j * m_terrainWidth + i;  // Note: Changed from m_terrainHeight to m_terrainWidth

	height = m_heightMap[index].y;
	normal = Vector3(m_heightMap[index].nx, m_heightMap[index].ny, m_heightMap[index].nz);

	return true;
}

bool Terrain::Update()
{
	return true;
}

float* Terrain::GetWavelength()
{
	return &m_wavelength;
}

float* Terrain::GetAmplitude()
{
	return &m_amplitude;
}