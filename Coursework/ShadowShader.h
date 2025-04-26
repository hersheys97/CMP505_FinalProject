#ifndef _SHADOWSHADER_H_
#define _SHADOWSHADER_H_

#include "DXF.h"

using namespace std;
using namespace DirectX;


class ShadowShader : public BaseShader
{
private:
	struct MatrixBufferType
	{
		XMMATRIX world;
		XMMATRIX view;
		XMMATRIX projection;
		XMMATRIX lightView;
		XMMATRIX lightProjection;
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

		XMFLOAT3 directionalLightDirection;
		float timeVal;
		XMFLOAT4 directionalLightColour;

		float specularPower;
		XMFLOAT3 moonPos;

		XMFLOAT4 specularColour;
	};

public:

	ShadowShader(ID3D11Device* device, HWND hwnd);
	~ShadowShader();

	void setShaderParameters(ID3D11DeviceContext* deviceContext, const XMMATRIX& world, const XMMATRIX& view, const XMMATRIX& projection, ID3D11ShaderResourceView* texture, ID3D11ShaderResourceView* depthMap, Light* light, float timeVal, float* pointLight_pos1, float* pointLight1Colour, float pointLight1Radius, float* pointLight_pos2, float* pointLight2Colour, float pointLight2Radius, float* directionalLight_dir, float* directionalLightColour, float* moonPos);

private:
	void initShader(const wchar_t* vs, const wchar_t* ps);

private:
	ID3D11Buffer* matrixBuffer;
	ID3D11SamplerState* sampleState;
	ID3D11SamplerState* sampleStateShadow;
	ID3D11Buffer* lightBuffer;
};

#endif