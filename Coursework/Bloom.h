#pragma once
#pragma once

#include "DXF.h"

using namespace std;
using namespace DirectX;

class Bloom : public BaseShader
{
private:
	struct BloomBufferType
	{
		float blurAmount;
		float bloomIntensity;
		float screenWidth;
		float screenHeight;
		float time;
		XMFLOAT3 padding;
	};

public:
	Bloom(ID3D11Device* device, HWND hwnd);
	~Bloom();

	void setShaderParameters(ID3D11DeviceContext* deviceContext, const XMMATRIX& world, const XMMATRIX& view, const XMMATRIX& projection, ID3D11ShaderResourceView* texture, float screenWidth, float screenHeight, SceneData* sceneData);

private:
	void initShader(const wchar_t* vs, const wchar_t* ps);

private:
	ID3D11Buffer* matrixBuffer;
	ID3D11SamplerState* sampleState;
	ID3D11Buffer* bloomBuffer;
};