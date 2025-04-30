/*

Collision Detection:

A simple sinusoidal terrain (no external libs) and hooked up smooth collision-sliding:

1) Procedural height & normal (getHeight/getNormal) use a sine‐cosine field in TerrainManipulation (inspired by Andy Gibson’s height‐provider concept).
2) Camera collision + sliding in App1::render() clamps the camera above terrainY + eyeHeight and then projects out the into-terrain component of the velocity to slide along the surface (per BraynzarSoft’s swept-sphere and Gamedev.SE’s vector projection trick).

*/


#pragma once

#include "DXF.h"
#include "VoronoiIslands.h"

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

	VoronoiIslands voronoiIslands;

	ID3D11Buffer* matrixBuffer;
	ID3D11Buffer* lightBuffer;
	ID3D11Buffer* cameraBuffer;

	ID3D11SamplerState* terrainSampleState;
	ID3D11SamplerState* textureSamplerState;
	ID3D11SamplerState* shadowSample1;
	ID3D11SamplerState* shadowSample2;

	void initShader(const wchar_t* cs, const wchar_t* ps);
	void initShader(const wchar_t* vsFilename, const wchar_t* hsFilename, const wchar_t* dsFilename, const wchar_t* psFilename);

	static constexpr float HEIGHT_AMPLITUDE = 1.0f;
	static constexpr float HEIGHT_FREQ = 0.1f;
	static constexpr float NORMAL_DELTA = 0.1f;

	const float TERRAIN_MIN_X = -10.f;
	const float TERRAIN_MAX_X = 10.f;
	const float TERRAIN_MIN_Z = -10.f;
	const float TERRAIN_MAX_Z = 10.f;


public:

	TerrainManipulation(ID3D11Device* device, HWND hwnd);
	~TerrainManipulation();

	float getHeight(float x, float z) const;
	bool isOnTerrain(float x, float z) const;
	XMFLOAT3 getNormal(float x, float z) const;

	void setShaderParameters(ID3D11DeviceContext* deviceContext, const XMMATRIX& world, const XMMATRIX& view, const XMMATRIX& projection, ID3D11ShaderResourceView* terrain, ID3D11ShaderResourceView* texture_height, ID3D11ShaderResourceView* texture_colour, ID3D11ShaderResourceView* texture_colour1, ID3D11ShaderResourceView* depth1, ID3D11ShaderResourceView* depth2, Camera* camera, Light* light, Light* directionalLight, SceneData* sceneData);
};