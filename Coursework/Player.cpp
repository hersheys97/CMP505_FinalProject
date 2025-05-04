#include "Player.h"
#include <algorithm>

static float distPointLineSegment2D(
	float px, float pz,
	float x0, float z0,
	float x1, float z1)
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
	rotation({ 0.f, 0.f, 0.f }),
	speed(20.0f),
	camEyeHeight(1.8f),
	jumpForce(7.0f),
	mouseSensitivity(0.5f),
	isJumping(false)
{
}

void Player::update(float deltaTime, Input* input, TerrainManipulation* terrain) {
	XMFLOAT3 moveInput = { 0.f, 0.f, 0.f };
	if (input->isKeyDown('W')) moveInput.z += 1.0f;
	if (input->isKeyDown('S')) moveInput.z -= 1.0f;
	if (input->isKeyDown('A')) moveInput.x -= 1.0f;
	if (input->isKeyDown('D')) moveInput.x += 1.0f;
	if (input->isKeyDown(VK_SPACE) && !isJumping) {
		velocity.y = jumpForce;
		isJumping = true;
	}

	XMVECTOR moveVec = XMLoadFloat3(&moveInput);
	if (!XMVector3Equal(moveVec, XMVectorZero())) {
		moveVec = XMVector3Normalize(moveVec);
		XMStoreFloat3(&moveInput, moveVec);
	}

	XMMATRIX rotMatrix = XMMatrixRotationY(XMConvertToRadians(rotation.y));
	XMVECTOR rotatedMove = XMVector3Transform(moveVec, rotMatrix);
	XMFLOAT3 worldMove;
	XMStoreFloat3(&worldMove, rotatedMove);

	bool isOnTerrain = terrain->isOnTerrain(position.x, position.z);
	float terrainHeight = isOnTerrain ? terrain->getHeight(position.x, position.z) : -50.f;
	bool isGrounded = isOnTerrain && (position.y <= (terrainHeight + camEyeHeight + 0.1f));

	bool isOnBridge = terrain->onBridge(position.x, position.z);
	float bridgeHeight = isOnBridge ? terrain->getHeight(position.x, position.z) : -50.f;
	bool isGrounded2 = isOnBridge && (position.y <= (bridgeHeight + camEyeHeight + 0.1f));

	if (!isGrounded && !isGrounded2) {
		velocity.y -= 15.8f * deltaTime;

		// Allow control in air (weaker)
		const float airControlFactor = 0.5f;
		velocity.x += worldMove.x * speed * airControlFactor * deltaTime;
		velocity.z += worldMove.z * speed * airControlFactor * deltaTime;

		// Stronger air drag to reduce sliding
		velocity.x *= 0.92f;
		velocity.z *= 0.92f;
	}
	else {
		if (velocity.y < 0.0f) {
			velocity.y = 0.0f;
			isJumping = false;
		}

		if (moveInput.x != 0.0f || moveInput.z != 0.0f) {
			velocity.x = worldMove.x * speed;
			velocity.z = worldMove.z * speed;
		}
		else {
			// Ground friction
			velocity.x *= 0.6f;
			velocity.z *= 0.6f;
		}
	}

	XMFLOAT3 newPos = {
		position.x + velocity.x * deltaTime,
		position.y + velocity.y * deltaTime,
		position.z + velocity.z * deltaTime
	};

	setPosition(newPos.x, newPos.y, newPos.z);
	if (position.y < -50.0f) resetParams();
}

void Player::handleMouseLook(Input* input, float deltaTime, HWND hwnd, int winW, int winH) {
	POINT p;
	GetCursorPos(&p);
	ScreenToClient(hwnd, &p);

	int centerX = winW / 2;
	int centerY = winH / 2;
	float dx = float(p.x - centerX);
	float dy = float(p.y - centerY);

	rotation.y += dx * mouseSensitivity;
	rotation.x += dy * mouseSensitivity;
	rotation.x = clamp(rotation.x, -89.0f, 89.0f);

	POINT warp = { centerX, centerY };
	ClientToScreen(hwnd, &warp);
	SetCursorPos(warp.x, warp.y);
}

void Player::resetParams() {
	velocity = { 0.f, 0.f, 0.f };
	rotation = { 0.f, 0.f, 0.f };
	isJumping = false;
}

void Player::setPosition(float x, float y, float z) {
	position = XMFLOAT3(x, y, z);
}

XMFLOAT3 Player::getPosition() const {
	return position;
}

XMFLOAT3 Player::getRotation() const {
	return rotation;
}