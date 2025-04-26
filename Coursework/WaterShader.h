#pragma once
#include "DXF.h"

using namespace std;
using namespace DirectX;

class WaterShader : public BaseShader
{
private:
	struct MatrixBufferType
	{
		XMMATRIX world;
		XMMATRIX view;
		XMMATRIX projection;
		XMMATRIX lightView;
		XMMATRIX lightProjection;
		XMMATRIX lightView2;
		XMMATRIX lightProjection2;
	};

	struct CameraBufferType
	{
		XMFLOAT3 cameraPosition;
		float padding;
	};

	struct TimeBufferType
	{
		float time;
		float amplitude;
		float frequency;
		float speed;
	};

	struct LightBufferType
	{
		XMFLOAT4 ambientColour;
		XMFLOAT4 diffuseColour;

		XMFLOAT3 pointLight1Position;
		float pointLight1Radius;
		XMFLOAT4 pointLight1Colour;

		XMFLOAT3 pointLight2Position;
		float pointLight2Radius;
		XMFLOAT4 pointLight2Colour;

		XMFLOAT4 spotlightColour;

		float spotlightCutoff;
		XMFLOAT3 spotlightDirection;

		float spotlightFalloff;
		XMFLOAT3 moonPos;

		XMFLOAT4 specularColour;
		float specularPower;
		XMFLOAT3 directionalLightDirection;

		XMFLOAT4 directionalColour;
	};

public:
	WaterShader(ID3D11Device* device, HWND hwnd);
	~WaterShader();

	void setShaderParameters(ID3D11DeviceContext* deviceContext, const XMMATRIX& worldMatrix, const XMMATRIX& viewMatrix, const XMMATRIX& projectionMatrix, ID3D11ShaderResourceView* water, ID3D11ShaderResourceView* depthMap, ID3D11ShaderResourceView* depthMap2, Camera* camera, Light* light, Light* directionalLight, SceneData* sceneData);

private:
	void initShader(const wchar_t* cs, const wchar_t* ps);

private:
	ID3D11Buffer* matrixBuffer;
	ID3D11Buffer* cameraBuffer;
	ID3D11Buffer* timeBuffer;
	ID3D11Buffer* lightBuffer;

	ID3D11SamplerState* waterSampleState;
	ID3D11SamplerState* sampleStateShadow;
	ID3D11SamplerState* sampleStateShadow2;
};