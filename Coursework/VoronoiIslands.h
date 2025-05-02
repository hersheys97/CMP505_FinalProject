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
	struct Wall {
		XMFLOAT3 start;
		XMFLOAT3 end;
		float height;
	};

	struct Island {
		XMFLOAT3 position = { 0.f, 0.f, 0.f };
		float rotationY = 0.f;
		bool initialized = false;
		vector<Wall> walls;
	};

	struct Bridge {
		size_t islandA;
		size_t islandB;
	};

	VoronoiIslands(int cellSize, int islandCount);
	void GenerateIslands();

	const vector<Island>& GetIslands() const { return islands; }
	const vector<Bridge>& GetBridges() const { return bridges; }

	XMFLOAT3 GetRandomIslandPosition() const {
		if (islands.empty()) return { 0, 0, 0 };

		uniform_int_distribution<size_t> dist(0, islands.size() - 1);
		size_t randomIndex = dist(*gen);

		// Return just the XZ position, we'll get height from terrain later
		return {
			islands[randomIndex].position.x,
			0.0f, // Y will be set from terrain height
			islands[randomIndex].position.z
		};
	}

private:
	void GenerateVoronoiRegions();
	void GenerateMinimumSpanningTree();
	void GenerateWalls(Island& island);

	vector<Island> islands;
	vector<Bridge> bridges;
	vector<XMFLOAT3> voronoiRegions;
	int gridSize;
	unique_ptr<mt19937> gen;
};