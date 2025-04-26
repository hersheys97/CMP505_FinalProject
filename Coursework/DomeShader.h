#pragma once

#include "DXF.h"
#include "SceneData.h"

using namespace std;
using namespace DirectX;

class DomeShader : public BaseShader
{
private:
	struct MatrixBufferType
	{
		XMMATRIX world;
		XMMATRIX view;
		XMMATRIX projection;
	};

public:
	DomeShader(ID3D11Device* device, HWND hwnd);
	~DomeShader();

	void setShaderParameters(ID3D11DeviceContext* deviceContext, const XMMATRIX& world, const XMMATRIX& view, const XMMATRIX& projection, ID3D11ShaderResourceView* dome, SceneData* sceneData);

private:
	void initShader(const wchar_t* cs, const wchar_t* ps);

private:
	ID3D11Buffer* matrixBuffer;
	ID3D11SamplerState* domeSampleState;
	ID3D11SamplerState* sampleStateShadow;
};