#include "TeapotSpotlight.h"

TeapotSpotlight::TeapotSpotlight() :
	cutoffAngle(cosf(XMConvertToRadians(45.0f))),
	falloff(2.0f),
	radius(8.0f),
	pos{ 0,0,0 },
	dir{ 0,-1,0 }
{
}

void TeapotSpotlight::Initialize()
{
	setDiffuseColour(1.0f, 0.9f, 0.7f, 1.0f);
	setAmbientColour(0.2f, 0.2f, 0.2f, 1.0f);
	setSpecularColour(1.0f, 1.0f, 1.0f, 1.0f);
	setSpecularPower(32.0f);
	isActive = false;
}

void TeapotSpotlight::Update()
{
	if (isActive)
	{
		pos = XMFLOAT3(targetPosition.x, targetPosition.y + 3.0f, targetPosition.z);
		dir = XMFLOAT3(0.0f, -1.0f, 0.0f);

		Light::setPosition(pos.x, pos.y, pos.z);
		Light::setDirection(dir.x, dir.y, dir.z);
	}
}

XMMATRIX TeapotSpotlight::GetViewMatrix() const
{
	XMVECTOR positionVec = XMLoadFloat3(&pos);
	XMVECTOR targetVec = XMLoadFloat3(&targetPosition);
	XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	return XMMatrixLookAtLH(positionVec, targetVec, up);
}

XMMATRIX TeapotSpotlight::GetProjectionMatrix() const
{
	return XMMatrixPerspectiveFovLH(cutoffAngle * 2.0f, 1.0f, 0.1f, radius);
}