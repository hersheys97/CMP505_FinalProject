#pragma once

#include "DXF.h"

using namespace std;
using namespace DirectX;

class ChromaticAberration : public BaseShader
{
private:
	struct MatrixBufferType
	{
		XMMATRIX world;
		XMMATRIX view;
		XMMATRIX projection;
	};

	struct AberrationBufferType
	{
		XMFLOAT2 chromaticOffset;
		XMFLOAT2 ghostScreenPos;
		float ghostDistance;
		float timeVal;
		float effectIntensity;
		float padding;
	};

public:
	ChromaticAberration(ID3D11Device* device, HWND hwnd);
	~ChromaticAberration();

	void setShaderParameters(ID3D11DeviceContext* deviceContext, const XMMATRIX& worldMatrix, const XMMATRIX& orthoView, const XMMATRIX& orthoMatrix, ID3D11ShaderResourceView* texture, XMFLOAT2 offsets, XMFLOAT2 ghostScreenPos, float ghostDistance, float timeVal, float effectIntensity);

private:
	void initShader(const wchar_t* vs, const wchar_t* ps);

private:
	ID3D11Buffer* matrixBuffer;
	ID3D11Buffer* aberrationBuffer;
	ID3D11SamplerState* sampleState;
};