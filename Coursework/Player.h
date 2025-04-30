#pragma once
#include <BaseApplication.h>
#include "TerrainManipulation.h"

class Player {
public:
	Player();

	XMFLOAT3 getPosition() const;
	XMFLOAT3 getRotation() const;

	void update(float deltaTime, Input* input, TerrainManipulation* terrain);
	void resetPosition(TerrainManipulation* terrain);
	void setPosition(float x, float y, float z);
	void handleMouseLook(Input* input, float deltaTime, HWND hwnd, int winW, int winH);

	XMFLOAT3 getCameraPosition() const {
		return XMFLOAT3(position.x, position.y + camEyeHeight, position.z);
	}

	XMFLOAT3 getCameraTarget() const {
		float yawRad = XMConvertToRadians(rotation.y);
		float pitchRad = XMConvertToRadians(rotation.x);
		float forwardX = sinf(yawRad) * cosf(pitchRad);
		float forwardY = sinf(pitchRad);
		float forwardZ = cosf(yawRad) * cosf(pitchRad);
		return XMFLOAT3(position.x + forwardX, position.y + camEyeHeight + forwardY, position.z + forwardZ);
	}

private:
	XMFLOAT3 position;
	XMFLOAT3 velocity;
	XMFLOAT3 rotation;
	float speed;
	float camEyeHeight;
	float jumpForce;
	float mouseSensitivity;
	bool isJumping;
};