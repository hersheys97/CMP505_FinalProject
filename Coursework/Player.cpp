#include "Player.h"
#include <algorithm>

static float distPointLineSegment2D(float px, float pz, float x0, float z0, float x1, float z1)
{
	float vx = x1 - x0, vz = z1 - z0;
	float wx = px - x0, wz = pz - z0;
	float c1 = vx * wx + vz * wz;
	float c2 = vx * vx + vz * vz;
	float t = (c2 < 1e-6f) ? 0.f : c1 / c2;
	t = max(0.f, min(1.f, t));
	float ix = x0 + t * vx, iz = z0 + t * vz;
	float dx = px - ix, dz = pz - iz;
	return std::sqrt(dx * dx + dz * dz);
}

template<typename T>
T clamp(T v, T lo, T hi) {
	return (v < lo) ? lo : (v > hi) ? hi : v;
}

Player::Player() :
	position({ 58.881f, 8.507f, 68.2f }),
	velocity({ 0.f, 0.f, 0.f }),
	rotation({ 0.f, 0.0f, 0.f }),
	speed(40.0f),
	camEyeHeight(3.f),
	jumpForce(7.0f),
	mouseSensitivity(0.4f),
	isJumping(false),
	sceneData(nullptr)
{
}

void Player::initialize(SceneData* sceneData) {
	this->sceneData = sceneData;
	if (sceneData) {
		sceneData->playerData.firstTimeInPlayMode = true;
		camEyeHeight = sceneData->playerData.cameraEyeHeight;
	}
}

XMFLOAT3 Player::getCameraTarget() const {
	const float yawRad = XMConvertToRadians(rotation.y);
	const float pitchRad = XMConvertToRadians(rotation.x);
	const XMFLOAT3 camPos = getCameraPosition();

	return {
		camPos.x + sinf(yawRad) * cosf(pitchRad),
		camPos.y + sinf(pitchRad),
		camPos.z + cosf(yawRad) * cosf(pitchRad)
	};
}

bool Player::isGrounded(TerrainManipulation* terrain) const {
	const bool isOnTerrain = terrain->isOnTerrain(position.x, position.z);
	const float terrainHeight = isOnTerrain ? terrain->getHeight(position.x, position.z) : -50.f;
	const bool terrainGrounded = isOnTerrain && (position.y <= (terrainHeight + camEyeHeight + 0.1f));

	const bool isOnBridge = terrain->onBridge(position.x, position.z);
	const float bridgeHeight = isOnBridge ? terrain->getHeight(position.x, position.z) : -50.f;
	const bool bridgeGrounded = isOnBridge && (position.y <= (bridgeHeight + camEyeHeight + 0.1f));

	return terrainGrounded || bridgeGrounded;
}

void Player::update(float deltaTime, Input* input, TerrainManipulation* terrain) {
	// Process movement input
	XMFLOAT3 moveInput = {};
	if (input->isKeyDown('W')) moveInput.z += 1.0f;
	if (input->isKeyDown('S')) moveInput.z -= 1.0f;
	if (input->isKeyDown('A')) moveInput.x -= 1.0f;
	if (input->isKeyDown('D')) moveInput.x += 1.0f;
	if (input->isKeyDown(VK_SPACE) && !isJumping) {
		velocity.y = jumpForce;
		isJumping = true;
	}

	// Normalize and rotate movement vector
	XMVECTOR moveVec = XMLoadFloat3(&moveInput);
	if (!XMVector3Equal(moveVec, XMVectorZero())) {
		moveVec = XMVector3Normalize(XMVector3Transform(
			moveVec,
			XMMatrixRotationY(XMConvertToRadians(rotation.y))
		));
		XMStoreFloat3(&moveInput, moveVec);
	}

	// Physics update
	const bool grounded = isGrounded(terrain);
	if (!grounded) {
		// Airborne physics
		velocity.y -= 15.8f * deltaTime;
		constexpr float airControlFactor = 0.5f;
		velocity.x += moveInput.x * speed * airControlFactor * deltaTime;
		velocity.z += moveInput.z * speed * airControlFactor * deltaTime;
		velocity.x *= 0.92f; // Air resistance
		velocity.z *= 0.92f;
	}
	else {
		// Grounded physics
		if (velocity.y < 0.0f) {
			velocity.y = 0.0f;
			isJumping = false;
		}

		if (moveInput.x != 0.0f || moveInput.z != 0.0f) {
			velocity.x = moveInput.x * speed;
			velocity.z = moveInput.z * speed;
		}
		else {
			velocity.x *= 0.8f; // Ground friction
			velocity.z *= 0.8f;
		}
	}

	// Update position
	position.x += velocity.x * deltaTime;
	position.y += velocity.y * deltaTime;
	position.z += velocity.z * deltaTime;

	if (position.y < -50.0f) {
		resetParams();
	}
}
void Player::updatePlayer(float deltaTime, Input* input, TerrainManipulation* terrain, Camera* camera, FMODAudioSystem* audioSystem, Islands* islands)
{
	update(deltaTime, input, terrain);
	updateCameraPosition(camera);
	handleTerrainCollision(deltaTime, terrain, camera);
	HandlePickupCollisions(islands, audioSystem);

	// Update audio listener
	const XMFLOAT3 camPos = camera->getPosition();
	const XMFLOAT3 camForward = camera->getForward();
	const XMFLOAT3 camUp = camera->getUp();
	audioSystem->updateListenerPosition(camPos, camForward, camUp);
}

void Player::handleMouseLook(Input* input, float deltaTime, HWND hwnd, int winW, int winH)
{
	POINT cursorPos;
	GetCursorPos(&cursorPos);
	ScreenToClient(hwnd, &cursorPos);

	const int centerX = winW / 2;
	const int centerY = winH / 2;
	const float dx = static_cast<float>(cursorPos.x - centerX);
	const float dy = static_cast<float>(cursorPos.y - centerY);

	rotation.y += dx * mouseSensitivity;
	rotation.x = clamp(rotation.x + dy * mouseSensitivity, -89.0f, 89.0f);

	// Reset cursor to center
	POINT center = { centerX, centerY };
	ClientToScreen(hwnd, &center);
	SetCursorPos(center.x, center.y);
}

void Player::handleTerrainCollision(float deltaTime, TerrainManipulation* terrain, Camera* camera)
{
	XMFLOAT3 camPos = camera->getPosition();

	if (terrain->isOnTerrain(camPos.x, camPos.z)) {
		const float terrainY = terrain->getHeight(camPos.x, camPos.z);
		const float minY = terrainY + camEyeHeight;

		if (camPos.y < minY) {
			camPos.y = minY;

			// Handle sliding
			const XMFLOAT3 normal = terrain->getNormal(camPos.x, camPos.z);
			const XMVECTOR N = XMLoadFloat3(&normal);
			const XMVECTOR velocityVec = XMLoadFloat3(&sceneData->playerData.cameraVelocity);

			// Calculate projection
			const float dotProduct = XMVectorGetX(XMVector3Dot(velocityVec, N));
			const XMVECTOR proj = XMVectorScale(N, dotProduct);

			// Calculate slide velocity
			const XMVECTOR slideVel = XMVectorSubtract(velocityVec, proj);
			XMFLOAT3 sv;
			XMStoreFloat3(&sv, slideVel);

			// Update position
			camPos.x = sceneData->playerData.lastCameraPosition.x + sv.x * deltaTime;
			camPos.z = sceneData->playerData.lastCameraPosition.z + sv.z * deltaTime;

			// Apply changes
			camera->setPosition(camPos.x, camPos.y, camPos.z);
			XMStoreFloat3(&sceneData->playerData.cameraVelocity, slideVel);
		}
	}

	// Always update last position
	sceneData->playerData.lastCameraPosition = camPos;
}

void Player::HandlePickupCollisions(Islands* islands, FMODAudioSystem* audioSystem) {
	if (!islands) return;

	XMFLOAT3 pickupPosition;
	const float playerCollisionRadius = 3.0f;

	if (islands->CheckPickupCollision(position, playerCollisionRadius, pickupPosition)) audioSystem->playOneShot("event:/Pickup");
}

void Player::handlePlayModeReset(Islands* islands, TerrainManipulation* terrain, Camera* camera)
{
	if (!sceneData) return;

	if (sceneData->playerData.firstTimeInPlayMode || position.y < -50.0f) {
		const XMFLOAT3 islandPos = islands->GetRandomIslandPosition();
		const float terrainHeight = terrain->getHeight(islandPos.x, islandPos.z);

		setPosition(islandPos.x, terrainHeight + 2.0f, islandPos.z);

		const XMFLOAT3 camPos = getCameraPosition();
		camera->setPosition(camPos.x, camPos.y + camEyeHeight, camPos.z);
		camera->setRotation(0.0f, 0.0f, 0.0f);

		if (sceneData->playerData.firstTimeInPlayMode) {
			sceneData->playerData.firstTimeInPlayMode = false;
		}
	}
}

void Player::handleSonar(Input* input, FMODAudioSystem* audioSystem)
{
	if (!sceneData || !input->isKeyDown('C') || sceneData->sonarData.isActive) {
		return;
	}

	sceneData->sonarData = { true,0.0f,sceneData->sonarData.sonarDuration,position };
	sceneData->ghostData.sonarTargetPosition = sceneData->sonarData.sonarOrigin;
	sceneData->ghostData.respondingToSonar = true;
	sceneData->ghostData.sonarResponseTimer = 0.0f;
	sceneData->tessMesh = true;

	audioSystem->playOneShot("event:/EchoPulse");
	audioSystem->dimBGM(FMODAudioSystem::ECHO_EFFECT_DURATION);
}

void Player::updateCameraPosition(Camera* camera)
{
	const XMFLOAT3 camPos = getCameraPosition();
	camera->setPosition(camPos.x, camPos.y + camEyeHeight, camPos.z);
	camera->setRotation(rotation.x, rotation.y, 0.0f);
	camera->update();
}

void Player::resetParams()
{
	velocity = { 0.f, 0.f, 0.f };
	rotation = { 0.f, 0.0f, 0.f };
	isJumping = false;
}

void Player::setPosition(float x, float y, float z)
{
	position = { x, y, z };
}