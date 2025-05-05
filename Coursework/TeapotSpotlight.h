#pragma once
#include "DXF.h"
#include "Light.h"

class TeapotSpotlight : public Light
{
private:
	float cutoffAngle;
	float falloff;
	float radius;
	XMFLOAT3 pos;
	XMFLOAT3 dir;

public:
	bool isActive = false;
	XMFLOAT3 targetPosition;

	TeapotSpotlight();

	void Update();
	void Initialize();

	// Getters with const correctness
	XMFLOAT3 getPos() const { return pos; }
	XMFLOAT3 getDir() const { return dir; }

	void setSpotlightCutoff(float cutoff) { cutoffAngle = cutoff; }
	float getSpotlightCutoff() const { return cutoffAngle; }

	void setSpotlightFalloff(float falloff) { this->falloff = falloff; }
	float getSpotlightFalloff() const { return falloff; }

	void setLightRadius(float radius) { this->radius = radius; }
	float getLightRadius() const { return radius; }

	XMMATRIX GetViewMatrix() const;
	XMMATRIX GetProjectionMatrix() const;
};