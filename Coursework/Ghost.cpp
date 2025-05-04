#include "Ghost.h"
#include <algorithm>

Ghost::Ghost() :sceneData(nullptr), audioSystem(nullptr), voronoiIslands(nullptr)
{
}

void Ghost::Initialize(FMODAudioSystem* audioSys, SceneData* sceneData) {
	audioSystem = audioSys;
	this->sceneData = sceneData;
}

void Ghost::Update(float deltaTime, const XMFLOAT3& playerPosition) {
	if (!sceneData) return;

	if (sceneData->ghostData.isActive && !sceneData->ghostData.respondingToSonar) sceneData->ghostData.aliveTime -= deltaTime;

	if ((!sceneData->ghostData.isActive || sceneData->ghostData.aliveTime <= 0.f) &&
		voronoiIslands && !voronoiIslands->GetIslands().empty()) {
		Respawn();
	}
	else {
		if (sceneData->ghostData.respondingToSonar) UpdateSonarResponse(deltaTime);
		else HandleNormalWandering(deltaTime);

		UpdatePosition(deltaTime);
		UpdateChromaticAberration(playerPosition);
	}

	if (sceneData->ghostData.isActive) UpdateAudio(deltaTime, playerPosition);
	else if (audioSystem->isWhisperPlaying()) audioSystem->stopGhostWhisper();
}

bool Ghost::IsActive() const {
	return sceneData->ghostData.isActive;
}

void Ghost::HandleSonar(const XMFLOAT3& sonarPosition, float sonarDuration) {
	sceneData->ghostData.respondingToSonar = true;
	sceneData->ghostData.sonarResponseTimer = 0.0f;
	sceneData->ghostData.sonarTargetPosition = sonarPosition;
	sceneData->sonarData.sonarDuration = sonarDuration;
}

void Ghost::Respawn() {
	if (audioSystem->isWhisperPlaying()) audioSystem->stopGhostWhisper();

	if (!sceneData->ghostData.respondingToSonar &&
		voronoiIslands &&
		!voronoiIslands->GetIslands().empty()) {

		const std::vector<Voronoi::Island>& islands = voronoiIslands->GetIslands();
		sceneData->ghostData.currentIslandIndex = rand() % islands.size();
		const Voronoi::Island& island = islands[sceneData->ghostData.currentIslandIndex];

		sceneData->ghostData.position = { island.position.x + (rand() % 10 - 5), island.position.y + 3.f, island.position.z + (rand() % 10 - 5) };
		sceneData->ghostData.velocity = { randomFloat(-1.f, 1.f) * 2.f,0.f,randomFloat(-1.f, 1.f) * 2.f };
		sceneData->ghostData.maxLifetime = 30.0f;
		sceneData->ghostData.aliveTime = sceneData->ghostData.maxLifetime;
		sceneData->ghostData.isActive = true;
		sceneData->ghostData.directionChangeTimer = 0.0f;
		sceneData->ghostData.nextDirectionChangeTime = randomFloat(3.f, 7.f);
	}
}

void Ghost::UpdateSonarResponse(float deltaTime) {
	if (!sceneData->ghostData.respondingToSonar) return;

	sceneData->ghostData.sonarResponseTimer += deltaTime;

	XMVECTOR targetPos = XMLoadFloat3(&sceneData->ghostData.sonarTargetPosition);
	XMVECTOR currentPos = XMLoadFloat3(&sceneData->ghostData.position);
	XMVECTOR toTarget = XMVectorSubtract(targetPos, currentPos);
	float distance = XMVectorGetX(XMVector3Length(toTarget));

	if (distance < 0.5f || sceneData->ghostData.sonarResponseTimer >= sceneData->sonarData.sonarDuration) {
		static XMFLOAT3 preSonarVelocity;
		static float preSonarDirectionChangeTimer;
		static float preSonarNextDirectionChangeTime;

		if (distance < 0.5f) {
			// Save pre-sonar state
			preSonarVelocity = sceneData->ghostData.velocity;
			preSonarDirectionChangeTimer = sceneData->ghostData.directionChangeTimer;
			preSonarNextDirectionChangeTime = sceneData->ghostData.nextDirectionChangeTime;
		}

		sceneData->ghostData.respondingToSonar = false;
		sceneData->ghostData.sonarResponseTimer = 0.0f;
		Respawn();

		// Restore pre-sonar state if target reached
		if (distance < 0.5f) {
			sceneData->ghostData.velocity = preSonarVelocity;
			sceneData->ghostData.directionChangeTimer = preSonarDirectionChangeTimer;
			sceneData->ghostData.nextDirectionChangeTime = preSonarNextDirectionChangeTime;
		}
	}
	else {
		float remainingTime = sceneData->sonarData.sonarDuration - sceneData->ghostData.sonarResponseTimer;
		float requiredSpeed = (remainingTime > 0) ? (distance / remainingTime) : 0.f;

		XMVECTOR direction = XMVector3Normalize(toTarget);
		XMStoreFloat3(&sceneData->ghostData.velocity, XMVectorScale(direction, requiredSpeed));
	}
}

void Ghost::HandleNormalWandering(float deltaTime) {
	if (!voronoiIslands ||
		sceneData->ghostData.currentIslandIndex < 0 ||
		sceneData->ghostData.currentIslandIndex >= voronoiIslands->GetIslands().size()) {
		return;
	}

	const auto& island = voronoiIslands->GetIslands()[sceneData->ghostData.currentIslandIndex];
	float halfSize = 25.0f;
	float minX = island.position.x - halfSize;
	float maxX = island.position.x + halfSize;
	float minZ = island.position.z - halfSize;
	float maxZ = island.position.z + halfSize;

	bool bounced = HandleBoundaryBounce(minX, maxX, minZ, maxZ);
	if (bounced) {
		sceneData->ghostData.directionChangeTimer = 0.0f;
		sceneData->ghostData.nextDirectionChangeTime = randomFloat(5.0f, 10.0f);
	}

	sceneData->ghostData.directionChangeTimer += deltaTime;
	if (sceneData->ghostData.directionChangeTimer >= sceneData->ghostData.nextDirectionChangeTime) {
		UpdateWanderingDirection();
		sceneData->ghostData.directionChangeTimer = 0.0f;
		sceneData->ghostData.nextDirectionChangeTime = randomFloat(5.0f, 10.0f);
	}
}

bool Ghost::HandleBoundaryBounce(float minX, float maxX, float minZ, float maxZ) {
	bool bounced = false;

	if (sceneData->ghostData.position.x < minX || sceneData->ghostData.position.x > maxX) {
		sceneData->ghostData.velocity.x *= -1.f;
		sceneData->ghostData.position.x = max(minX, min(maxX, sceneData->ghostData.position.x));
		bounced = true;
	}
	if (sceneData->ghostData.position.z < minZ || sceneData->ghostData.position.z > maxZ) {
		sceneData->ghostData.velocity.z *= -1.f;
		sceneData->ghostData.position.z = max(minZ, min(maxZ, sceneData->ghostData.position.z));
		bounced = true;
	}

	return bounced;
}

void Ghost::UpdateAudio(float deltaTime, const XMFLOAT3& listenerPosition) {
	if (!sceneData->ghostData.isActive) return;

	// Update 3D audio position
	audioSystem->updateGhostPosition(sceneData->ghostData.position);

	// Update volume based on listener distance
	audioSystem->updateGhostWhisperVolume(listenerPosition);

	// Update special audio effects
	audioSystem->updateGhostEffects(deltaTime, listenerPosition);
}

void Ghost::UpdateWanderingDirection() {
	sceneData->ghostData.velocity = { randomFloat(5.f, 8.f),0.f,randomFloat(5.f, 8.f) };
}

void Ghost::UpdatePosition(float deltaTime) {
	sceneData->ghostData.position.x += sceneData->ghostData.velocity.x * deltaTime;
	sceneData->ghostData.position.z += sceneData->ghostData.velocity.z * deltaTime;
}

void Ghost::UpdateChromaticAberration(const XMFLOAT3& playerPosition) {
	XMVECTOR ghostPos = XMLoadFloat3(&sceneData->ghostData.position);
	XMVECTOR playerPosVec = XMLoadFloat3(&playerPosition);
	float distance = XMVectorGetX(XMVector3Length(XMVectorSubtract(ghostPos, playerPosVec)));

	sceneData->chromaticAberrationData.maxIntensity = 0.01f;
	sceneData->chromaticAberrationData.enabled = distance <= 50.0f;

	if (sceneData->chromaticAberrationData.enabled) sceneData->chromaticAberrationData.intensity = sceneData->chromaticAberrationData.maxIntensity * (1.0f - (distance / 50.0f)); // Linear falloff
	else sceneData->chromaticAberrationData.intensity = 0.0f;
}

void Ghost::Render(const XMMATRIX& worldMatrix, const XMMATRIX& viewMatrix,
	const XMMATRIX& projectionMatrix, int sceneWidth,
	int sceneHeight, const XMFLOAT3& cameraPosition) {
	if (!sceneData->ghostData.isActive) return;

	// Screen position calculation (same as old code)
	XMVECTOR ghostPos = XMLoadFloat3(&sceneData->ghostData.position);
	XMMATRIX viewProj = viewMatrix * projectionMatrix;
	XMVECTOR screenPos = XMVector3Project(ghostPos, 0, 0, sceneWidth, sceneHeight,
		0.0f, 1.0f, projectionMatrix, viewMatrix, XMMatrixIdentity());

	XMFLOAT3 ghostScreenPos;
	XMStoreFloat3(&ghostScreenPos, screenPos);

	// Normalized screen coordinates (same as old code)
	sceneData->chromaticAberrationData.ghostScreenPos.x = ghostScreenPos.x / sceneWidth;
	sceneData->chromaticAberrationData.ghostScreenPos.y = ghostScreenPos.y / sceneHeight;

	// Distance calculation (same as old code)
	XMVECTOR camPosVec = XMLoadFloat3(&cameraPosition);
	XMVECTOR distanceVec = XMVector3Length(XMVectorSubtract(ghostPos, camPosVec));
	float maxDistance = 50.0f;
	sceneData->chromaticAberrationData.ghostDistance =
		1.0f - min(1.0f, XMVectorGetX(distanceVec) / maxDistance);

	// Time-based effects (EXACTLY as in old code)
	chromaticTimeAccumulator += 0.016f; // Assuming 60fps
	sceneData->chromaticAberrationData.timeCalc = chromaticTimeAccumulator;
	sceneData->chromaticAberrationData.effectIntensity =
		sceneData->chromaticAberrationData.intensity;

	// Wave effect parameters (EXACTLY as in old code)
	float offsetAngle = chromaticTimeAccumulator * 2.0f; // Original multiplier was 2.0
	float offsetMagnitude = sceneData->chromaticAberrationData.effectIntensity * 0.05f; // Original multiplier was 0.05
	sceneData->chromaticAberrationData.offsets.x = cos(offsetAngle) * offsetMagnitude;
	sceneData->chromaticAberrationData.offsets.y = sin(offsetAngle) * offsetMagnitude;
}

float randomFloat(float min, float max) {
	return min + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / (max - min)));
}