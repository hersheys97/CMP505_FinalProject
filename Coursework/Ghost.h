#pragma once
#include <DirectXMath.h>
#include "FMODAudioSystem.h"
#include "Islands.h"
#include "SceneData.h"

// Forward declarations
struct GhostData;
struct ChromaticAberrationData;

class Ghost {
public:
	Ghost();
	~Ghost() = default;

	void Initialize(FMODAudioSystem* audioSystem, SceneData* sceneData);
	void Update(float deltaTime, const XMFLOAT3& playerPosition);
	void Render(const XMMATRIX& worldMatrix, const XMMATRIX& viewMatrix, const XMMATRIX& projectionMatrix, int sceneWidth, int sceneHeight, const XMFLOAT3& cameraPosition);
	void HandleSonar(const XMFLOAT3& sonarPosition, float sonarDuration);

	bool IsActive() const;
	const GhostData& GetData() const { return sceneData->ghostData; }
	void SetIslandBounds(Islands* islands) { islandBounds = islands; }

private:
	void Respawn();
	void UpdateSonarResponse(float deltaTime);
	void HandleNormalWandering(float deltaTime);
	bool HandleBoundaryBounce(float minX, float maxX, float minZ, float maxZ);
	void UpdateWanderingDirection();
	void UpdatePosition(float deltaTime);
	void UpdateChromaticAberration(const XMFLOAT3& playerPosition);
	void UpdateAudio(float deltaTime, const XMFLOAT3& listenerPosition);

	SceneData* sceneData;
	FMODAudioSystem* audioSystem;
	Islands* islandBounds;
	float chromaticTimeAccumulator = 0.0f;
};

float randomFloat(float min, float max);