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
		if (minX < regionMinX) islands[i].position.x += (regionMinX - minX);
		if (maxX > regionMaxX)  islands[i].position.x -= (maxX - regionMaxX);
		if (minZ < regionMinZ)  islands[i].position.z += (regionMinZ - minZ);
		if (maxZ > regionMaxZ)  islands[i].position.z -= (maxZ - regionMaxZ);

		islands[i].initialized = true;

		GenerateWalls(islands[i]);
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

		const XMFLOAT3& start = islands[parent[i]].position;
		const XMFLOAT3& end = islands[i].position;
	}
}

void VoronoiIslands::GenerateWalls(Island& island) {
	const float ISLAND_SIZE = 50.f;
	const int MIN_WALLS = 2;
	const int MAX_WALLS = 5;
	const float WALL_HEIGHT = 10.f;
	const float MIN_WALL_LENGTH = 5.f;
	const float MAX_WALL_LENGTH = 20.f;
	const float WALL_SAFE_MARGIN = 5.f; // Minimum distance between walls

	uniform_int_distribution<int> wallCountDist(MIN_WALLS, MAX_WALLS);
	uniform_real_distribution<float> angleDist(0.f, XM_2PI);
	uniform_real_distribution<float> offsetDist(-ISLAND_SIZE * 0.7f, ISLAND_SIZE * 0.7f); // More conservative bounds

	int wallCount = wallCountDist(*gen);
	vector<Wall> tempWalls;
	const int MAX_ATTEMPTS = 50; // Prevent infinite loops

	for (int i = 0; i < wallCount; ++i) {
		bool validWall = false;
		int attempts = 0;

		while (!validWall && attempts < MAX_ATTEMPTS) {
			attempts++;

			// Generate random wall parameters
			float angle = angleDist(*gen);
			float length = uniform_real_distribution<float>(
				MIN_WALL_LENGTH,
				min(MAX_WALL_LENGTH, ISLAND_SIZE * 1.4f) // Don't exceed reasonable length
			)(*gen);

			// Generate start point with safe margins
			float offsetX = offsetDist(*gen);
			float offsetZ = offsetDist(*gen);

			// Calculate start and end points in local space
			XMFLOAT3 localStart = { offsetX, 0.f, offsetZ };
			XMFLOAT3 localEnd = {
				localStart.x + cosf(angle) * length,
				0.f,
				localStart.z + sinf(angle) * length
			};

			// Check if both points are within island bounds
			if (fabs(localStart.x) > ISLAND_SIZE || fabs(localStart.z) > ISLAND_SIZE ||
				fabs(localEnd.x) > ISLAND_SIZE || fabs(localEnd.z) > ISLAND_SIZE) {
				continue; // Try again if out of bounds
			}

			// Transform points to world space for overlap checking
			XMMATRIX transform = XMMatrixRotationY(island.rotationY) *
				XMMatrixTranslation(island.position.x, island.position.y, island.position.z);

			XMVECTOR worldStart = XMVector3TransformCoord(XMLoadFloat3(&localStart), transform);
			XMVECTOR worldEnd = XMVector3TransformCoord(XMLoadFloat3(&localEnd), transform);

			// Ensure transformed coordinates are valid
			XMFLOAT3 transformedStart, transformedEnd;
			XMStoreFloat3(&transformedStart, worldStart);
			XMStoreFloat3(&transformedEnd, worldEnd);

			if (std::isnan(transformedStart.x) || std::isnan(transformedStart.z) ||
				std::isnan(transformedEnd.x) || std::isnan(transformedEnd.z)) {
				continue;  // Skip invalid walls
			}

			// Check for overlaps with existing walls
			bool overlaps = false;
			for (const auto& existingWall : tempWalls) {
				XMVECTOR eStart = XMLoadFloat3(&existingWall.start);
				XMVECTOR eEnd = XMLoadFloat3(&existingWall.end);

				// Simple distance check between wall segments
				if (XMVectorGetX(XMVector3Length(worldStart - eStart)) < WALL_SAFE_MARGIN ||
					XMVectorGetX(XMVector3Length(worldStart - eEnd)) < WALL_SAFE_MARGIN ||
					XMVectorGetX(XMVector3Length(worldEnd - eStart)) < WALL_SAFE_MARGIN ||
					XMVectorGetX(XMVector3Length(worldEnd - eEnd)) < WALL_SAFE_MARGIN) {
					overlaps = true;
					break;
				}
			}

			if (!overlaps) {
				Wall wall;
				wall.height = WALL_HEIGHT;
				XMStoreFloat3(&wall.start, worldStart);
				XMStoreFloat3(&wall.end, worldEnd);
				tempWalls.push_back(wall);
				validWall = true;
			}
		}
	}

	// Commit valid walls to the island
	island.walls = move(tempWalls);
}
