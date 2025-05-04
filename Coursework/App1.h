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
#include "ChromaticAberration.h"
#include "DepthShader.h"
#include "GhostShader.h"
#include "WaterDepthShader.h"
#include "TerrainDepthShader.h"
#include "SimpleTexture.h"
#include "SceneData.h"
#include "Player.h"
#include "VoronoiIslands.h"
#include "FMODAudioSystem.h"

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
	void initComponents();

	float randomFloat(float min, float max);

	// Rendering methods
	void renderToTexture();
	void finalRenderToScreen();

	void renderAudio();
	void getShadowDepthMap(const XMMATRIX& worldMatrix, const XMMATRIX& viewMatrix, const XMMATRIX& projectionMatrix, const XMMATRIX& identityMatrix);
	void finalRender(const XMMATRIX& worldMatrix, const XMMATRIX& viewMatrix, const XMMATRIX& projectionMatrix, const XMMATRIX& identityMatrix);
	void renderPlayer();
	void renderGhost(const XMMATRIX& worldMatrix, const XMMATRIX& viewMatrix, const XMMATRIX& projectionMatrix);

	void applyBloom(const XMMATRIX& worldMatrix, const XMMATRIX& viewMatrix, const XMMATRIX& projectionMatrix);
	void applyChromaticAberration(const XMMATRIX& worldMatrix, const XMMATRIX& orthoViewMatrix, const XMMATRIX& orthoMatrix);

	void renderDome(const XMMATRIX& worldMatrix, const XMMATRIX& viewMatrix, const XMMATRIX& projectionMatrix);
	void renderTerrain(const XMMATRIX& worldMatrix, const XMMATRIX& viewMatrix, const XMMATRIX& projectionMatrix);
	void renderWater(const XMMATRIX& worldMatrix, const XMMATRIX& viewMatrix, const XMMATRIX& projectionMatrix);
	void renderMoon(const XMMATRIX& worldMatrix, const XMMATRIX& viewMatrix, const XMMATRIX& projectionMatrix);
	void renderFirefly(const XMMATRIX& worldMatrix, const XMMATRIX& viewMatrix, const XMMATRIX& projectionMatrix);

	void generateIslands(const XMMATRIX& worldMatrix, const XMMATRIX& viewMatrix, const XMMATRIX& projectionMatrix);
	void generateBridges(const XMMATRIX& worldMatrix, const XMMATRIX& viewMatrix, const XMMATRIX& projectionMatrix);
	void generatePickups(const XMMATRIX& worldMatrix, const XMMATRIX& viewMatrix, const XMMATRIX& projectionMatrix);

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

	// Render Texture - Chromatic Aberration
	bool useChromaticAberration = false;
	float chromaticAberrationIntensity = 0.0f;
	float maxChromaticAberration = 0.01f; // Adjust for stronger/weaker effect
	XMFLOAT2 offsets = { 0.0f, 0.0f };
	XMFLOAT2 ghostScreenPos = { 0.0f, 0.0f };
	float ghostDistance = 0.0f;
	float timeCalc = 0.0f;
	float effectIntensity = 0.0f;

	ChromaticAberration* chromaticAberration;
	RenderTexture* renderAberration;
	SimpleTexture* simpleTexture;
	QuadMesh* screenEffects;

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

	// Model - Ghost
	AModel* ghost;
	GhostShader* ghostShader;

	// Lights
	Light* spotLight;
	Light* directionalLight;
	Light* pointLight1;
	Light* pointLight2;

	// Model - Pickups
	AModel* teapot;

	// Player
	Player* player;
	XMFLOAT3 m_lastCamPos = { 0.f,0.f,0.f };
	XMVECTOR m_camVelocity = XMVectorZero();
	float m_camEyeHeight = 1.8f;
	bool firstTimeInPlayMode = true;

	// Voronoi Islands
	unique_ptr<Voronoi::VoronoiIslands> voronoiIslands;
	float islandSize = 50.0f; // ISLAND_SIZE
	int islandCount = 2;
	float minIslandDistance = 30.0f;
	int gridSize = 700;

	// FMOD
	FMODAudioSystem audioSystem;
	bool startedBGM = false;
	bool firstTimeGeneratingIslands = true;

	// Echo Pulse
	bool sonarActive = false;
	float sonarTime = 0.0f;
	float sonarDuration = 3.0f;
	XMFLOAT3 sonarOrigin = { 0,0,0 };
	float sonarMaxRadius = 80.0f;

	// Ghost
	XMFLOAT3 ghostPosition = { 0.f, 0.f, 0.f };
	XMFLOAT3 ghostVelocity = { 0.f, 0.f, 0.f };
	float ghostLifetime = 0.f;
	const float ghostMaxLifetime = 15.f;
	bool ghostActive = false;
	int currentIslandIndex = -1;
	float directionChangeTimer = 0.0f;
	float nextDirectionChangeTime = 7.0;

	float effectFadeTimer = 0.0f;
	float effectFadeDuration = 1.0f; // 1 second fade in/out
	bool effectFadingIn = false;
	bool effectFadingOut = false;

	XMFLOAT3 sonarTargetPosition = { 0.f, 0.f, 0.f };
	float sonarResponseTimer = 0.f;
	const float sonarResponseDuration = 5.0f; // 5 secs
	bool respondingToSonar = false;

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