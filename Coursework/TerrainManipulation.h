#pragma once

#include "DXF.h"

using namespace std;
using namespace DirectX;

class TerrainManipulation : public BaseShader
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
	TerrainManipulation(ID3D11Device* device, HWND hwnd);
	~TerrainManipulation();

	void setShaderParameters(ID3D11DeviceContext* deviceContext, const XMMATRIX& world, const XMMATRIX& view, const XMMATRIX& projection, ID3D11ShaderResourceView* terrain, ID3D11ShaderResourceView* texture_height, ID3D11ShaderResourceView* texture_colour, ID3D11ShaderResourceView* texture_colour1, ID3D11ShaderResourceView* depth1, ID3D11ShaderResourceView* depth2, Camera* camera, Light* light, Light* directionalLight, SceneData* sceneData);

private:
	void initShader(const wchar_t* cs, const wchar_t* ps);
	void initShader(const wchar_t* vsFilename, const wchar_t* hsFilename, const wchar_t* dsFilename, const wchar_t* psFilename);

private:
	ID3D11Buffer* matrixBuffer;
	ID3D11Buffer* lightBuffer;
	ID3D11Buffer* cameraBuffer;

	ID3D11SamplerState* terrainSampleState;
	ID3D11SamplerState* textureSamplerState;
	ID3D11SamplerState* shadowSample1;
	ID3D11SamplerState* shadowSample2;
};