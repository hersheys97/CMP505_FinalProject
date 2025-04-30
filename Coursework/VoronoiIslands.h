#pragma once
#include "DXF.h"
#include <vector>
#include <DirectXMath.h>
#include <random>
#include <memory>

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

	VoronoiIslands(int cellSize, int islandCount);
	void GenerateIslands();
	const vector<Island>& GetIslands() const { return islands; }
	const vector<Bridge>& GetBridges() const { return bridges; }

private:
	void GenerateVoronoiRegions();
	void GenerateMinimumSpanningTree();

	vector<Island> islands;
	vector<Bridge> bridges;
	vector<XMFLOAT3> voronoiRegions;
	int gridSize;
	unique_ptr<mt19937> gen;
};