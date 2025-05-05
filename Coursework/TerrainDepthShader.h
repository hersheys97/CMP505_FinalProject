#pragma once

#include "DXF.h"

using namespace std;
using namespace DirectX;

class TerrainDepthShader : public BaseShader
{
private:
	struct MatrixBufferType
	{
		XMMATRIX world;
		XMMATRIX view;
		XMMATRIX projection;
	};

	struct CameraBufferType
	{
		XMFLOAT3 cameraPosition;
		float padding;
	};


public:
	TerrainDepthShader(ID3D11Device* device, HWND hwnd);
	~TerrainDepthShader();

	void setShaderParameters(ID3D11DeviceContext* deviceContext, const XMMATRIX& world, const XMMATRIX& view, const XMMATRIX& projection, ID3D11ShaderResourceView* terrain, ID3D11ShaderResourceView* texture_height, Camera* camera);

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

	ID3D11RasterizerState* rasterState;
};