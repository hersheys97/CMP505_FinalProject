#pragma once
#include "DXF.h"

using namespace std;
using namespace DirectX;

class WaterDepthShader : public BaseShader
{
private:
	struct TimeBufferType
	{
		float time;
		float amplitude;
		float frequency;
		float speed;
	};

public:
	WaterDepthShader(ID3D11Device* device, HWND hwnd);
	~WaterDepthShader();

	void setShaderParameters(ID3D11DeviceContext* deviceContext, const XMMATRIX& world, const XMMATRIX& view, const XMMATRIX& projection, SceneData* sceneData);

private:
	void initShader(const wchar_t* vs, const wchar_t* ps);

private:
	ID3D11Buffer* matrixBuffer;
	ID3D11Buffer* timeBuffer;
};