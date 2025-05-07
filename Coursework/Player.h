#pragma once
#include <BaseApplication.h>
#include "SceneData.h"
#include "TerrainManipulation.h"
#include "FMODAudioSystem.h"

class Player {
public:
	Player();
	~Player() = default;

	// Initialization
	void initialize(SceneData* sceneData);

	// Core gameplay functions
	void updatePlayer(float deltaTime, Input* input, TerrainManipulation* terrain,
		Camera* camera, FMODAudioSystem* audioSystem, Islands* islands);

	// Movement and camera
	void update(float deltaTime, Input* input, TerrainManipulation* terrain);
	void handleMouseLook(Input* input, float deltaTime, HWND hwnd, int winW, int winH);

	// Gameplay systems
	void handleSonar(Input* input, FMODAudioSystem* audioSystem);
	void handlePlayModeReset(Islands* islands, TerrainManipulation* terrain, Camera* camera);
	void HandlePickupCollisions(Islands* islands, FMODAudioSystem* audioSystem);

	// State management
	void resetParams();
	void setPosition(float x, float y, float z);

	// Getters
	const XMFLOAT3& getPosition() const { return position; }
	const XMFLOAT3& getRotation() const { return rotation; }
	XMFLOAT3 getCameraPosition() const { return { position.x, position.y + camEyeHeight, position.z }; }
	XMFLOAT3 getCameraTarget() const;

private:
	// Helper methods
	void updateCameraPosition(Camera* camera);
	void handleTerrainCollision(float deltaTime, TerrainManipulation* terrain, Camera* camera);
	bool isGrounded(TerrainManipulation* terrain) const;

	// Member variables
	SceneData* sceneData = nullptr;
	XMFLOAT3 position = { 58.881f, 8.507f, 68.2f };
	XMFLOAT3 velocity = { 0.f, 0.f, 0.f };
	XMFLOAT3 rotation = { 0.f, 90.f, 0.f };

	// Configuration
	float speed = 20.0f;
	float camEyeHeight = 3.f;
	float jumpForce = 7.0f;
	float mouseSensitivity = 0.5f;
	bool isJumping = false;
};