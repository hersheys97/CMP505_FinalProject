#ifndef _GHOSTSHADER_H_
#define _GHOSTSHADER_H_

#include "DXF.h"
#include "TeapotSpotlight.h"

using namespace std;
using namespace DirectX;

class GhostShader : public BaseShader
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

		float timeVal;

		XMFLOAT2 padding;

		float padding2;
		XMFLOAT3 directionalLightDirection;

		XMFLOAT4 directionalColour;

	};

public:

	GhostShader(ID3D11Device* device, HWND hwnd);
	~GhostShader();

	void setShaderParameters(ID3D11DeviceContext* deviceContext, const XMMATRIX& worldMatrix, const XMMATRIX& viewMatrix, const XMMATRIX& projectionMatrix, ID3D11ShaderResourceView* texture, Camera* camera, Light* light, Light* directionalLight, SceneData* sceneData);

private:
	void initShader(const wchar_t* vs, const wchar_t* ps);

private:
	ID3D11Buffer* matrixBuffer;
	ID3D11Buffer* lightBuffer;
	ID3D11Buffer* cameraBuffer;

	ID3D11SamplerState* sampleState;
	ID3D11SamplerState* sampleStateShadow;
};

#endif