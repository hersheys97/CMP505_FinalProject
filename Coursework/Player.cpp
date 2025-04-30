#include "Player.h"
#include <algorithm>

template<typename T>
T clamp(T v, T lo, T hi) {
	return (v < lo) ? lo : (v > hi) ? hi : v;
}

Player::Player() :
	position({ 58.881f, 8.507f, 68.2f }),
	velocity({ 0.f, 0.f, 0.f }),
	rotation({ 0.f, 0.f, 0.f }),
	speed(15.0f),
	camEyeHeight(1.8f),
	jumpForce(7.0f),
	mouseSensitivity(0.2f),
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
	float terrainHeight = isOnTerrain ? terrain->getHeight(position.x, position.z) : -100.f;
	bool isGrounded = isOnTerrain && (position.y <= (terrainHeight + camEyeHeight + 0.1f));

	if (!isGrounded) {
		velocity.y -= 15.8f * deltaTime;
		velocity.x *= 0.95f;
		velocity.z *= 0.95f;
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
			velocity.x *= 0.8f;
			velocity.z *= 0.8f;
		}
	}

	XMFLOAT3 newPos = {
		position.x + velocity.x * deltaTime,
		position.y + velocity.y * deltaTime,
		position.z + velocity.z * deltaTime
	};

	bool newPosOnTerrain = terrain->isOnTerrain(newPos.x, newPos.z);
	float newTerrainHeight = newPosOnTerrain ? terrain->getHeight(newPos.x, newPos.z) : -100.f;
	float groundLevel = newTerrainHeight + camEyeHeight;

	if (newPosOnTerrain && newPos.y < groundLevel) {
		XMFLOAT3 normal = terrain->getNormal(newPos.x, newPos.z);
		XMVECTOR N = XMLoadFloat3(&normal);
		XMVECTOR vel = XMLoadFloat3(&velocity);
		XMVECTOR intoTerrain = XMVectorScale(N, XMVectorGetX(XMVector3Dot(vel, N)));
		XMVECTOR slideVel = XMVectorSubtract(vel, intoTerrain);
		XMFLOAT3 sv;
		XMStoreFloat3(&sv, slideVel);
		newPos.x = position.x + sv.x * deltaTime;
		newPos.z = position.z + sv.z * deltaTime;
		newPos.y = groundLevel;
		velocity.y = 0.0f;
		isJumping = false;
		XMStoreFloat3(&velocity, slideVel);
	}

	setPosition(newPos.x, newPos.y, newPos.z);
	if (position.y < -100.0f) resetPosition(terrain);
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

void Player::resetPosition(TerrainManipulation* terrain) {
	setPosition(58.881f, terrain->getHeight(58.881f, -68.2f) + camEyeHeight + 2.0f, 68.2f);
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
