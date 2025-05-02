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

	struct SonarBufferType {
		XMFLOAT3 sonarOrigin;
		float sonarRadius;
		bool sonarActive;
		XMFLOAT3 padding;
	};

	unique_ptr<VoronoiIslands> voronoiIslands;

	ID3D11Buffer* matrixBuffer;
	ID3D11Buffer* lightBuffer;
	ID3D11Buffer* cameraBuffer;
	ID3D11Buffer* sonarBuffer;

	ID3D11SamplerState* terrainSampleState;
	ID3D11SamplerState* textureSamplerState;
	ID3D11SamplerState* shadowSample1;
	ID3D11SamplerState* shadowSample2;

	void initShader(const wchar_t* cs, const wchar_t* ps);
	void initShader(const wchar_t* vsFilename, const wchar_t* hsFilename, const wchar_t* dsFilename, const wchar_t* psFilename);

	const vector<VoronoiIslands::Island>* m_islands = nullptr;
	float m_regionSize = 0.0f;

	const vector<VoronoiIslands::Wall>* m_walls = nullptr;
	vector<pair<XMFLOAT3, XMFLOAT3>> m_bridges; // start , end
	float m_bridgeWidth = 5.0f;

	static constexpr float HEIGHT_AMPLITUDE = 1.0f;
	static constexpr float HEIGHT_FREQ = 0.1f;
	static constexpr float NORMAL_DELTA = 0.1f;

	/*const float TERRAIN_MIN_X = -50.f;
	const float TERRAIN_MAX_X = 50.f;
	const float TERRAIN_MIN_Z = -50.f;
	const float TERRAIN_MAX_Z = 50.f;*/


public:

	TerrainManipulation(ID3D11Device* device, HWND hwnd);
	~TerrainManipulation();

	float getHeight(float x, float z) const;
	bool isOnTerrain(float x, float z) const;
	XMFLOAT3 getNormal(float x, float z) const;
	const vector<VoronoiIslands::Wall>& getWalls() const {
		return *m_walls;  // Returns a reference to the vector of walls
	}
	const vector<pair<XMFLOAT3, XMFLOAT3>>& getBridges() const {
		return m_bridges;  // Returns a reference to the vector of bridge pairs
	}

	void setIslands(const vector<VoronoiIslands::Island>& islands, float regionSize);

	void setWalls(const vector<VoronoiIslands::Wall>& walls) {
		m_walls = &walls;
	}
	void setBridges(const vector<VoronoiIslands::Bridge>& bridges,
		const vector<VoronoiIslands::Island>& islands)
	{
		m_bridges.clear();
		for (auto& b : bridges) {
			m_bridges.emplace_back(
				islands[b.islandA].position,
				islands[b.islandB].position
			);
		}
	}

	bool collidesWall(float x, float z) const;
	bool onBridge(float x, float z) const;


	void setShaderParameters(ID3D11DeviceContext* deviceContext, const XMMATRIX& world, const XMMATRIX& view, const XMMATRIX& projection,
		bool sonarActive, XMFLOAT3 sonarOrigin, float sonarRadius,
		ID3D11ShaderResourceView* terrain, ID3D11ShaderResourceView* texture_height, ID3D11ShaderResourceView* texture_colour, ID3D11ShaderResourceView* texture_colour1, ID3D11ShaderResourceView* depth1, ID3D11ShaderResourceView* depth2, Camera* camera, Light* light, Light* directionalLight, SceneData* sceneData);
};