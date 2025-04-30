#include "Player.h"
#include <algorithm>

template<typename T>
T clamp(T v, T lo, T hi)
{
	return (v < lo) ? lo : (v > hi) ? hi : v;
}

Player::Player() :
	position({ 17.f, 13.f, -9.f }),
	velocity({ 0.f, 0.f, 0.f }),
	rotation({ 0.f, 0.f, 0.f })
{
	speed = 5.0f;
	camEyeHeight = 1.8f;
	jumpForce = 7.0f;
	mouseSensitivity = 0.2f;
	isJumping = false;
}

void Player::update(float deltaTime, Input* input, TerrainManipulation* terrain)
{
	// --- Input Handling ---
	XMFLOAT3 moveInput = { 0.f, 0.f, 0.f };
	if (input->isKeyDown('W')) moveInput.z += 1.0f;
	if (input->isKeyDown('S')) moveInput.z -= 1.0f;
	if (input->isKeyDown('A')) moveInput.x -= 1.0f;
	if (input->isKeyDown('D')) moveInput.x += 1.0f;

	if (input->isKeyDown(VK_SPACE) && !isJumping)
	{
		velocity.y = jumpForce;
		isJumping = true;
	}

	XMVECTOR moveVec = XMLoadFloat3(&moveInput);
	if (!XMVector3Equal(moveVec, XMVectorZero()))
	{
		moveVec = XMVector3Normalize(moveVec);
		XMStoreFloat3(&moveInput, moveVec);
	}

	XMMATRIX rotMatrix = XMMatrixRotationY(XMConvertToRadians(rotation.y));
	XMVECTOR rotatedMove = XMVector3Transform(moveVec, rotMatrix);
	XMFLOAT3 worldMove;
	XMStoreFloat3(&worldMove, rotatedMove);

	// --- Physics ---
	bool onTerrain = terrain->isOnTerrain(position.x, position.z);
	float terrainHeight = 0.0f;
	float groundLevel = 0.0f;
	bool isGrounded = false;

	if (onTerrain)
	{
		terrainHeight = terrain->getHeight(position.x, position.z);
		groundLevel = terrainHeight + camEyeHeight;
		isGrounded = (position.y <= groundLevel + 0.1f);
	}

	if (!isGrounded)
	{
		velocity.y -= 15.8f * deltaTime; // Gravity
		velocity.x *= 0.95f;             // Air drag
		velocity.z *= 0.95f;
	}
	else
	{
		if (velocity.y < 0.0f)
		{
			velocity.y = 0.0f;
			isJumping = false;
		}

		// If grounded and moving input exists, apply horizontal input as velocity
		if (moveInput.x != 0.0f || moveInput.z != 0.0f)
		{
			velocity.x = worldMove.x * speed;
			velocity.z = worldMove.z * speed;
		}
		else
		{
			// Damp velocity when no input
			velocity.x *= 0.8f;
			velocity.z *= 0.8f;
		}
	}

	// --- Apply Movement ---

	setPosition(
		position.x + velocity.x * deltaTime,
		position.y + velocity.y * deltaTime,
		position.z + velocity.z * deltaTime
	);

	// --- Ground Collision ---
	if (terrain->isOnTerrain(position.x, position.z))
	{
		terrainHeight = terrain->getHeight(position.x, position.z);
		groundLevel = terrainHeight + camEyeHeight;

		if (position.y < groundLevel)
		{
			setPosition(position.x, groundLevel, position.z);
			velocity.y = 0.0f;
			isJumping = false;
		}
	}

	// --- Emergency Reset ---
	if (position.y < -1000.0f) resetPosition(terrain);
}


void Player::handleMouseLook(Input* /*input*/, float /*deltaTime*/, HWND hwnd, int winW, int winH)
{
	// 1) Get current cursor in *client* coords
	POINT p;
	GetCursorPos(&p);
	ScreenToClient(hwnd, &p);

	// 2) Compute delta from center
	int centerX = winW / 2;
	int centerY = winH / 2;
	float dx = float(p.x - centerX);
	float dy = float(p.y - centerY);

	// 3) Apply to rotation (no deltaTime here—mouse is already frame-independent)
	rotation.y += dx * mouseSensitivity;
	rotation.x += dy * mouseSensitivity;

	// 4) Clamp pitch
	rotation.x = clamp(rotation.x, -89.0f, 89.0f);

	// 5) Warp cursor back to center
	POINT warp = { centerX, centerY };
	ClientToScreen(hwnd, &warp);
	SetCursorPos(warp.x, warp.y);
}

void Player::resetPosition(TerrainManipulation* terrain)
{
	setPosition(17.f, terrain->getHeight(17.f, -9.f) + camEyeHeight + 2.0f, -9.f);

	velocity = { 0.f, 0.f, 0.f };
	rotation = { 0.f, 0.f, 0.f };
	isJumping = false;
}

void Player::setPosition(float x, float y, float z)
{
	position = XMFLOAT3(x, y, z);
}

XMFLOAT3 Player::getPosition() const
{
	return position;
}

XMFLOAT3 Player::getRotation() const
{
	return rotation;
}