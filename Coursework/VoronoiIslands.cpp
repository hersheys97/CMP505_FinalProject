#include "VoronoiIslands.h"
#include <cmath>

VoronoiIslands::VoronoiIslands(int cellSize, int islandCount)
	: gridSize(cellSize),
	gen(make_unique<mt19937>(random_device{}())) {
	islands.resize(islandCount);
}

void VoronoiIslands::GenerateIslands() {
	uniform_real_distribution<float> posDist(0.f, static_cast<float>(gridSize));
	uniform_real_distribution<float> rotDist(0.f, XM_2PI);

	for (auto& island : islands) {
		island.position = { posDist(*gen), 0.f, posDist(*gen) };
		island.rotationY = rotDist(*gen);
		island.initialized = true;
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