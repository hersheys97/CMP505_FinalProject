#pragma once
#include "DXF.h"
#include <vector>
#include <DirectXMath.h>
#include <random>
#include <memory>
#include <algorithm>

namespace Voronoi {
	// Constants
	constexpr float REGION_SIZE = 150.f;
	constexpr float ISLAND_SIZE = 50.f;
	constexpr float PICKUP_OFFSET_RATIO = 0.8f;

	struct Island {
		XMFLOAT3 position = { 0.f, 0.f, 0.f };
		float rotationY = 0.f;
		bool initialized = false;
		vector<XMFLOAT3> pickupPositions;
		bool hasAmbience = false;
	};

	struct Bridge {
		size_t islandA;
		size_t islandB;
	};

	class VoronoiIslands {
	public:
		VoronoiIslands(int cellSize, int islandCount);

		// Generation
		void GenerateIslands();

		// Getters
		const vector<Island>& GetIslands() const { return islands_; }
		vector<Island>& GetIslands() { return islands_; }
		const vector<Bridge>& GetBridges() const { return bridges_; }
		XMFLOAT3 GetRandomIslandPosition() const;

	private:
		// Core generation methods
		void GenerateVoronoiRegions();
		void GenerateMinimumSpanningTree();
		void GenerateIsland(Island& island, const XMFLOAT3& region);
		void GeneratePickups(Island& island);

		// Helper methods
		void CalculateGridDimensions(int& cols, int& rows) const;
		void AdjustIslandPosition(Island& island, const XMFLOAT3& region, float minX, float maxX, float minZ, float maxZ);
		void RotateIslandCorners(const Island& island, XMFLOAT3(&corners)[4]) const;

		// Data
		vector<Island> islands_;
		vector<Bridge> bridges_;
		vector<XMFLOAT3> voronoiRegions_;
		int gridSize_;
		unique_ptr<mt19937> randomEngine_;
	};
}