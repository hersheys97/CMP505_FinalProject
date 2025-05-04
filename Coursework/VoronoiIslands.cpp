#include "VoronoiIslands.h"

namespace Voronoi {
	VoronoiIslands::VoronoiIslands(int cellSize, int islandCount)
		: randomEngine_(make_unique<mt19937>(random_device{}())) {
		islands_.resize(islandCount);
		GenerateVoronoiRegions();
	}

	void VoronoiIslands::GenerateIslands() {
		uniform_real_distribution<float> posDist(-REGION_SIZE / 2.0f, REGION_SIZE / 2.0f);
		uniform_real_distribution<float> rotDist(0.f, XM_2PI);

		for (size_t i = 0; i < islands_.size(); ++i) {
			const XMFLOAT3& region = voronoiRegions_[i];
			islands_[i].position = { region.x + posDist(*randomEngine_), 0.0f, region.z + posDist(*randomEngine_) };
			islands_[i].rotationY = rotDist(*randomEngine_);

			GenerateIsland(islands_[i], region);
			GeneratePickups(islands_[i]);
		}

		GenerateMinimumSpanningTree();
	}

	void VoronoiIslands::GenerateIsland(Island& island, const XMFLOAT3& region) {
		XMFLOAT3 worldCorners[4];
		RotateIslandCorners(island, worldCorners);

		// Initialize with first corner
		float minX = worldCorners[0].x;
		float maxX = worldCorners[0].x;
		float minZ = worldCorners[0].z;
		float maxZ = worldCorners[0].z;

		// Check remaining corners
		for (int i = 1; i < 4; ++i) {
			minX = min(minX, worldCorners[i].x);
			maxX = max(maxX, worldCorners[i].x);
			minZ = min(minZ, worldCorners[i].z);
			maxZ = max(maxZ, worldCorners[i].z);
		}

		AdjustIslandPosition(island, region, minX, maxX, minZ, maxZ);
		island.initialized = true;
	}

	void VoronoiIslands::RotateIslandCorners(const Island& island, XMFLOAT3(&corners)[4]) const {
		const XMMATRIX rotMatrix = XMMatrixRotationY(island.rotationY);
		const XMVECTOR baseCorners[4] = {
			XMVectorSet(-ISLAND_SIZE, 0, -ISLAND_SIZE, 0),
			XMVectorSet(ISLAND_SIZE, 0, -ISLAND_SIZE, 0),
			XMVectorSet(ISLAND_SIZE, 0, ISLAND_SIZE, 0),
			XMVectorSet(-ISLAND_SIZE, 0, ISLAND_SIZE, 0)
		};

		for (int i = 0; i < 4; ++i) {
			XMStoreFloat3(&corners[i], XMVector3Transform(baseCorners[i], rotMatrix));
		}
	}

	void VoronoiIslands::AdjustIslandPosition(Island& island, const XMFLOAT3& region,
		float minX, float maxX, float minZ, float maxZ) {
		const float regionMinX = region.x - REGION_SIZE / 2.0f;
		const float regionMaxX = region.x + REGION_SIZE / 2.0f;
		const float regionMinZ = region.z - REGION_SIZE / 2.0f;
		const float regionMaxZ = region.z + REGION_SIZE / 2.0f;

		if (minX < regionMinX) island.position.x += (regionMinX - minX);
		if (maxX > regionMaxX) island.position.x -= (maxX - regionMaxX);
		if (minZ < regionMinZ) island.position.z += (regionMinZ - minZ);
		if (maxZ > regionMaxZ) island.position.z -= (maxZ - regionMaxZ);
	}

	void VoronoiIslands::GenerateVoronoiRegions() {
		if (islands_.empty()) return;

		int cols, rows;
		CalculateGridDimensions(cols, rows);

		voronoiRegions_.reserve(islands_.size());
		for (int i = 0; i < islands_.size(); ++i) {
			const int col = i % cols;
			const int row = i / cols;
			voronoiRegions_.emplace_back(
				(col * REGION_SIZE) + (REGION_SIZE / 2.0f),
				0.0f,
				(row * REGION_SIZE) + (REGION_SIZE / 2.0f)
			);
		}

		gridSize_ = static_cast<int>(max(cols, rows) * REGION_SIZE);
	}

	void VoronoiIslands::CalculateGridDimensions(int& cols, int& rows) const {
		cols = static_cast<int>(ceil(sqrt(islands_.size())));
		rows = (islands_.size() + cols - 1) / cols;
	}

	void VoronoiIslands::GenerateMinimumSpanningTree() {
		bridges_.clear();
		if (islands_.size() < 2) return;

		vector<vector<float>> distanceMatrix(islands_.size(), vector<float>(islands_.size()));
		for (size_t i = 0; i < islands_.size(); ++i) {
			for (size_t j = i + 1; j < islands_.size(); ++j) {
				const XMVECTOR diff = XMLoadFloat3(&islands_[j].position) - XMLoadFloat3(&islands_[i].position);
				distanceMatrix[i][j] = distanceMatrix[j][i] = XMVectorGetX(XMVector3Length(diff));
			}
		}

		vector<bool> inMST(islands_.size(), false);
		vector<size_t> parent(islands_.size(), 0);
		vector<float> key(islands_.size(), FLT_MAX);

		key[0] = 0;
		parent[0] = -1;

		for (size_t count = 0; count < islands_.size() - 1; ++count) {
			const size_t u = min_element(key.begin(), key.end()) - key.begin();
			inMST[u] = true;
			key[u] = FLT_MAX;

			for (size_t v = 0; v < islands_.size(); ++v) {
				if (!inMST[v] && distanceMatrix[u][v] < key[v]) {
					parent[v] = u;
					key[v] = distanceMatrix[u][v];
				}
			}
		}

		bridges_.reserve(islands_.size() - 1);
		for (size_t i = 1; i < islands_.size(); ++i) {
			bridges_.emplace_back(Bridge{ parent[i], i });
		}
	}

	void VoronoiIslands::GeneratePickups(Island& island) {
		uniform_int_distribution<int> countDist(1, 3);
		uniform_real_distribution<float> offsetDist(-ISLAND_SIZE * PICKUP_OFFSET_RATIO, ISLAND_SIZE * PICKUP_OFFSET_RATIO);
		uniform_real_distribution<float> heightDist(1.0f, 3.0f);

		const int pickupCount = countDist(*randomEngine_);
		island.pickupPositions.reserve(pickupCount);

		for (int i = 0; i < pickupCount; ++i) {
			float x = offsetDist(*randomEngine_);
			float z = offsetDist(*randomEngine_);

			if (x * x + z * z > ISLAND_SIZE * ISLAND_SIZE * PICKUP_OFFSET_RATIO * PICKUP_OFFSET_RATIO) {
				continue;
			}

			const XMMATRIX transform = XMMatrixRotationY(island.rotationY) *
				XMMatrixTranslation(island.position.x, island.position.y, island.position.z);

			XMFLOAT3 worldPosition;
			XMStoreFloat3(&worldPosition,
				XMVector3TransformCoord(XMVectorSet(x, heightDist(*randomEngine_), z, 1.0f), transform));

			island.pickupPositions.push_back(worldPosition);
		}
	}

	XMFLOAT3 VoronoiIslands::GetRandomIslandPosition() const {
		if (islands_.empty()) return { 0, 0, 0 };

		uniform_int_distribution<size_t> dist(0, islands_.size() - 1);
		const size_t randomIndex = dist(*randomEngine_);

		return { islands_[randomIndex].position.x, 0.0f, islands_[randomIndex].position.z };
	}

}