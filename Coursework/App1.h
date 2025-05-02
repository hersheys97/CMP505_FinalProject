// Application.h
#ifndef _APP1_H
#define _APP1_H

// Includes
#include "DXF.h"
#include "WaterShader.h"
#include "TerrainManipulation.h"
#include "MoonShader.h"
#include "DomeShader.h"
#include "Bloom.h"
#include "EchoPulse.h"
#include "DepthShader.h"
#include "FireflyShader.h"
#include "WaterDepthShader.h"
#include "TerrainDepthShader.h"
#include "SceneData.h"
#include "Player.h"
#include "VoronoiIslands.h"

#include "fmod_studio.hpp"
#include "fmod.hpp"
#include "fmod_errors.h"
#pragma comment(lib, "fmod_vc.lib") // Core FMOD library
#pragma comment(lib, "fmodstudio_vc.lib") // FMOD Studio library

enum class AppMode { FlyCam, Play };

extern AppMode currentMode;

class App1 : public BaseApplication
{
public:
	App1();
	~App1();

	void init(HINSTANCE hinstance, HWND hwnd, int screenWidth, int screenHeight, Input* in, bool vsyncEnabled, bool FULL_SCREEN) override;
	bool frame() override;

protected:
	bool render();
	void gui();

private:
	// Initialization method
	void initAudio();
	void initComponents();

	float randomFloat(float min, float max);

	// Rendering methods
	void renderAudio();
	void getShadowDepthMap(const XMMATRIX& worldMatrix, const XMMATRIX& viewMatrix, const XMMATRIX& projectionMatrix, const XMMATRIX& identityMatrix);
	void finalRender(const XMMATRIX& worldMatrix, const XMMATRIX& viewMatrix, const XMMATRIX& projectionMatrix, const XMMATRIX& identityMatrix);
	void renderPlayer();
	void renderGhost(const XMMATRIX& worldMatrix, const XMMATRIX& viewMatrix, const XMMATRIX& projectionMatrix);

	void applyBloom(const XMMATRIX& worldMatrix, const XMMATRIX& viewMatrix, const XMMATRIX& projectionMatrix);
	void applyPulse(const XMMATRIX& worldMatrix, const XMMATRIX& viewMatrix, const XMMATRIX& projectionMatrix);

	void renderDome(const XMMATRIX& worldMatrix, const XMMATRIX& viewMatrix, const XMMATRIX& projectionMatrix);
	void renderTerrain(const XMMATRIX& worldMatrix, const XMMATRIX& viewMatrix, const XMMATRIX& projectionMatrix);
	void renderWater(const XMMATRIX& worldMatrix, const XMMATRIX& viewMatrix, const XMMATRIX& projectionMatrix);
	void renderMoon(const XMMATRIX& worldMatrix, const XMMATRIX& viewMatrix, const XMMATRIX& projectionMatrix);
	void renderFirefly(const XMMATRIX& worldMatrix, const XMMATRIX& viewMatrix, const XMMATRIX& projectionMatrix);

	void generateIslands(const XMMATRIX& worldMatrix, const XMMATRIX& viewMatrix, const XMMATRIX& projectionMatrix);
	void generateBridges(const XMMATRIX& worldMatrix, const XMMATRIX& viewMatrix, const XMMATRIX& projectionMatrix);
	void generateWalls(const XMMATRIX& worldMatrix, const XMMATRIX& viewMatrix, const XMMATRIX& projectionMatrix);

	// Cleanup method
	void cleanup();

	HWND hwnd;

	// Shadows
	ShadowMap* shadowMap[2];
	DepthShader* depthShader;
	WaterDepthShader* waterDepthShader;
	TerrainDepthShader* terrainDepthShader;

	// Render Texture - Bloom
	RenderTexture* renderBloom;
	Bloom* bloomShader;

	// Render Texture - EchoPulse
	RenderTexture* renderTerrainRT;
	RenderTexture* renderEcho;
	EchoPulse* echoPulse;

	// Dome
	SphereMesh* circleDome;
	DomeShader* domeShader;

	// Water
	PlaneMesh* water;
	WaterShader* waterShader;

	// Terrain
	CubeMesh* topTerrain;
	TerrainManipulation* terrainShader;

	// Moon
	SphereMesh* moon;
	MoonShader* moonShader;

	// Model
	AModel* firefly;
	FireflyShader* fireflyShader;

	// Lights
	Light* spotLight;
	Light* directionalLight;
	Light* pointLight1;
	Light* pointLight2;

	// Player
	Player* player;
	XMFLOAT3 m_lastCamPos = { 0.f,0.f,0.f };
	XMVECTOR m_camVelocity = DirectX::XMVectorZero();
	float m_camEyeHeight = 1.8f;
	bool firstTimeInPlayMode = true;

	// Voronoi Islands
	unique_ptr<VoronoiIslands> voronoiIslands;
	float islandSize = 50.0f; // ISLAND_SIZE
	int islandCount = 2;
	float minIslandDistance = 30.0f;
	int gridSize = 700;

	// FMOD
	FMOD::Studio::System* studioSystem = nullptr;
	FMOD::Studio::EventInstance* bgm1Instance = nullptr;

	// Echo Pulse
	bool sonarActive = false;
	float sonarTime = 0.0f;
	float sonarDuration = 3.0f;
	XMFLOAT3 sonarOrigin = { 0,0,0 };
	float sonarMaxRadius = 80.0f;

	// Ghost
	XMFLOAT3 fireflyPosition = { 0.f, 0.f, 0.f };
	XMFLOAT3 fireflyVelocity = { 0.f, 0.f, 0.f };
	float fireflyLifetime = 0.f;
	const float fireflyMaxLifetime = 10.f;
	bool fireflyActive = false;
	int currentIslandIndex = -1;
	float directionChangeTimer = 0.0f;
	float nextDirectionChangeTime = 1.0f; // Initial interval

	// Screen dimensions
	float SCREEN_WIDTH = 0.f;
	float SCREEN_HEIGHT = 0.f;

	int shadowmapWidth = 1024;
	int shadowmapHeight = 1024;
	int sceneWidth = 100;
	int sceneHeight = 100;

	SceneData* sceneData;
};

#endif