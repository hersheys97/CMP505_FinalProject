#include "VoronoiIslands.h"
#include <cmath>
#include <iomanip>

VoronoiIslands::VoronoiIslands(int cellSize, int islandCount)
	: gen(make_unique<mt19937>(random_device{}())) {
	islands.resize(islandCount);
	GenerateVoronoiRegions();
}

void VoronoiIslands::GenerateVoronoiRegions() {
	const float REGION_SIZE = 150.f;
	int numIslands = static_cast<int>(islands.size());
	if (numIslands == 0) return;

	// Calculate grid dimensions
	int cols = static_cast<int>(ceil(sqrt(numIslands)));
	int rows = (numIslands + cols - 1) / cols;

	// Generate region centers
	for (int i = 0; i < numIslands; ++i) {
		int col = i % cols;
		int row = i / cols;
		voronoiRegions.emplace_back((col * REGION_SIZE) + (REGION_SIZE / 2.0f), 0.0f, (row * REGION_SIZE) + (REGION_SIZE / 2.0f));
	}

	// Set grid size to contain all regions
	gridSize = static_cast<int>(max(cols, rows) * REGION_SIZE);
}

void VoronoiIslands::GenerateIslands() {
	const float REGION_SIZE = 150.f;
	uniform_real_distribution<float> posDist(-REGION_SIZE / 2.0f, REGION_SIZE / 2.0f);
	uniform_real_distribution<float> rotDist(0.f, XM_2PI);

	for (size_t i = 0; i < islands.size(); ++i) {
		const XMFLOAT3& region = voronoiRegions[i];

		// Generate initial position with random offset within the Voronoi region
		islands[i].position = { region.x + posDist(*gen), 0.0f, region.z + posDist(*gen) };

		// Random rotation
		islands[i].rotationY = rotDist(*gen);

		// Define the half size of the island (since we are rotating around the center)
		const float ISLAND_SIZE = 50.f;

		// Create rotation matrix
		XMMATRIX rotMatrix = XMMatrixRotationY(islands[i].rotationY);

		// Define corners of the unrotated island (bounding box)
		XMVECTOR corners[4] = {
			XMVectorSet(-ISLAND_SIZE, 0, -ISLAND_SIZE, 0),
			XMVectorSet(ISLAND_SIZE, 0, -ISLAND_SIZE, 0),
			XMVectorSet(ISLAND_SIZE, 0, ISLAND_SIZE, 0),
			XMVectorSet(-ISLAND_SIZE, 0, ISLAND_SIZE, 0)
		};

		// Rotate the corners
		XMFLOAT3 worldCorners[4];
		for (int j = 0; j < 4; ++j) {
			XMVECTOR rotatedCorner = XMVector3Transform(corners[j], rotMatrix);
			XMStoreFloat3(&worldCorners[j], rotatedCorner);
		}

		// Now calculate the rotated bounding box dimensions
		float minX = worldCorners[0].x, maxX = worldCorners[0].x;
		float minZ = worldCorners[0].z, maxZ = worldCorners[0].z;
		for (int j = 1; j < 4; ++j) {
			minX = min(minX, worldCorners[j].x);
			maxX = max(maxX, worldCorners[j].x);
			minZ = min(minZ, worldCorners[j].z);
			maxZ = max(maxZ, worldCorners[j].z);
		}

		// Ensure the rotated island is within the Voronoi region
		float regionMinX = region.x - REGION_SIZE / 2.0f;
		float regionMaxX = region.x + REGION_SIZE / 2.0f;
		float regionMinZ = region.z - REGION_SIZE / 2.0f;
		float regionMaxZ = region.z + REGION_SIZE / 2.0f;

		// Adjust position if the island extends beyond the Voronoi region
		if (minX < regionMinX) {
			islands[i].position.x += (regionMinX - minX);
		}
		if (maxX > regionMaxX) {
			islands[i].position.x -= (maxX - regionMaxX);
		}
		if (minZ < regionMinZ) {
			islands[i].position.z += (regionMinZ - minZ);
		}
		if (maxZ > regionMaxZ) {
			islands[i].position.z -= (maxZ - regionMaxZ);
		}

		islands[i].initialized = true;
	}

	GenerateMinimumSpanningTree();
}


void VoronoiIslands::GenerateMinimumSpanningTree() {
	bridges.clear();
	if (islands.size() < 2) return;

	vector<vector<float>> distanceMatrix(islands.size(), vector<float>(islands.size()));
	for (size_t i = 0; i < islands.size(); ++i) {
		for (size_t j = i + 1; j < islands.size(); ++j) {
			XMVECTOR diff = XMLoadFloat3(&islands[j].position) - XMLoadFloat3(&islands[i].position);
			distanceMatrix[i][j] = distanceMatrix[j][i] = XMVectorGetX(XMVector3Length(diff));
		}
	}

	vector<bool> inMST(islands.size(), false);
	vector<size_t> parent(islands.size(), 0);
	vector<float> key(islands.size(), FLT_MAX);

	key[0] = 0;
	parent[0] = -1;

	for (size_t count = 0; count < islands.size() - 1; ++count) {
		size_t u = min_element(key.begin(), key.end()) - key.begin();
		inMST[u] = true;
		key[u] = FLT_MAX;

		for (size_t v = 0; v < islands.size(); ++v) {
			if (!inMST[v] && distanceMatrix[u][v] < key[v]) {
				parent[v] = u;
				key[v] = distanceMatrix[u][v];
			}
		}
	}

	for (size_t i = 1; i < islands.size(); ++i) {
		bridges.push_back(Bridge{ parent[i], i });
	}
}