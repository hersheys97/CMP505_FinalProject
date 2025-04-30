#pragma once
#include "DXF.h"
#include <vector>
#include <DirectXMath.h>
#include <random>
#include <memory>
#include <queue>

using namespace std;
using namespace DirectX;

class VoronoiIslands {
public:
	struct Island {
		XMFLOAT3 position = { 0.f, 0.f, 0.f };
		float rotationY = 0.f;
		bool initialized = false;
	};

	struct Bridge {
		size_t islandA;
		size_t islandB;
	};

	VoronoiIslands(int gridSize = 300, int islandCount = 2);
	void GenerateIslands();
	void SetGridSize(int size) { gridSize = max(size, 100); }
	const vector<Island>& GetIslands() const { return islands; }
	const vector<Bridge>& GetBridges() const { return bridges; }

private:
	void GenerateMinimumSpanningTree();

	vector<Island> islands;
	vector<Bridge> bridges;
	int gridSize;
	unique_ptr<mt19937> gen;
};