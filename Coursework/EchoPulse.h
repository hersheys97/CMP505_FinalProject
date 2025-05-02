#pragma once

#include "DXF.h"

using namespace std;
using namespace DirectX;

class EchoPulse : public BaseShader
{
private:
	struct MatrixBufferType
	{
		XMMATRIX world;
		XMMATRIX view;
		XMMATRIX projection;
	};

	struct PulseBufferType
	{
		float sonarMaxRadius;      // Maximum radius of sonar pulse
		float time;                // Time progression for the pulse
		float screenWidth;         // Screen width
		float screenHeight;        // Screen height
		XMFLOAT3 sonarOrigin;        // Sonar origin position
		bool sonarActive;          // Sonar pulse active state
	};

	struct HeatDistortionBufferType
	{
		float distortionStrength; // Amount of distortion (higher = stronger refraction)
		float distortionSpeed;    // Speed at which the distortion changes
		float time;              // Time value to animate the distortion
		float padding;
	};

public:
	EchoPulse(ID3D11Device* device, HWND hwnd);
	~EchoPulse();

	void setShaderParameters(ID3D11DeviceContext* deviceContext, const XMMATRIX& worldMatrix, const XMMATRIX& viewMatrix, const XMMATRIX& projectionMatrix, ID3D11ShaderResourceView* texture, bool sonarActive, const XMFLOAT3 sonarOrigin, float sonarMaxRadius, float sonarTime, float screenWidth, float screenHeight, XMFLOAT3 cameraPosition, float timeVal, SceneData* sceneData);

private:
	void initShader(const wchar_t* vs, const wchar_t* ps);

private:
	ID3D11Buffer* matrixBuffer;
	ID3D11Buffer* pulseBuffer;
	ID3D11Buffer* heatDistortionBuffer;
	ID3D11SamplerState* sampleState;
};