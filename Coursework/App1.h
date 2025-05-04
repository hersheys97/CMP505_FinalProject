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
#include "Ghost.h"

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

	// Rendering pipeline methods
	void renderToTexture();
	void finalRenderToScreen();
	void getShadowDepthMap(const XMMATRIX& worldMatrix, const XMMATRIX& viewMatrix, const XMMATRIX& projectionMatrix, const XMMATRIX& identityMatrix);
	void finalRender(const XMMATRIX& worldMatrix, const XMMATRIX& viewMatrix, const XMMATRIX& projectionMatrix, const XMMATRIX& identityMatrix);

	// Entity rendering methods
	void renderPlayer();
	void renderGhost(const XMMATRIX& worldMatrix, const XMMATRIX& viewMatrix, const XMMATRIX& projectionMatrix);
	void renderDome(const XMMATRIX& worldMatrix, const XMMATRIX& viewMatrix, const XMMATRIX& projectionMatrix);
	void renderTerrain(const XMMATRIX& worldMatrix, const XMMATRIX& viewMatrix, const XMMATRIX& projectionMatrix);
	void renderWater(const XMMATRIX& worldMatrix, const XMMATRIX& viewMatrix, const XMMATRIX& projectionMatrix);
	void renderMoon(const XMMATRIX& worldMatrix, const XMMATRIX& viewMatrix, const XMMATRIX& projectionMatrix);

	// Ghost behavior methods
	void handleGhostRespawn();
	void updateSonarResponse(float deltaTime);
	void handleNormalWandering(float deltaTime);
	bool handleBoundaryBounce(float minX, float maxX, float minZ, float maxZ);
	void updateWanderingDirection();
	void updateGhostPosition(float deltaTime);
	void renderGhostModel(const XMMATRIX& worldMatrix, const XMMATRIX& viewMatrix, const XMMATRIX& projectionMatrix, float deltaTime);

	// Post-processing methods
	void applyBloom(const XMMATRIX& worldMatrix, const XMMATRIX& viewMatrix, const XMMATRIX& projectionMatrix);
	void applyChromaticAberration(const XMMATRIX& worldMatrix, const XMMATRIX& orthoViewMatrix, const XMMATRIX& orthoMatrix);
	void updateChromaticAberration();

	// Audio methods
	void renderAudio();
	void updateGhostAudio(float deltaTime);

	// World generation methods
	void generateIslands(const XMMATRIX& worldMatrix, const XMMATRIX& viewMatrix, const XMMATRIX& projectionMatrix);
	void generateBridges(const XMMATRIX& worldMatrix, const XMMATRIX& viewMatrix, const XMMATRIX& projectionMatrix);
	void generatePickups(const XMMATRIX& worldMatrix, const XMMATRIX& viewMatrix, const XMMATRIX& projectionMatrix);

	// Cleanup
	void cleanup();

	// Hardware/Window handles
	HWND hwnd;

	// Rendering components
	ShadowMap* shadowMap[2];
	RenderTexture* renderBloom;
	RenderTexture* renderAberration;
	QuadMesh* screenEffects;

	// Shaders
	DepthShader* depthShader;
	WaterDepthShader* waterDepthShader;
	TerrainDepthShader* terrainDepthShader;
	Bloom* bloomShader;
	ChromaticAberration* chromaticAberration;
	SimpleTexture* simpleTexture;
	DomeShader* domeShader;
	WaterShader* waterShader;
	TerrainManipulation* terrainShader;
	MoonShader* moonShader;
	GhostShader* ghostShader;

	// Geometry
	SphereMesh* circleDome;
	PlaneMesh* water;
	CubeMesh* topTerrain;
	SphereMesh* moon;
	AModel* ghost;
	AModel* teapot;

	// Lighting
	Light* spotLight;
	Light* directionalLight;
	Light* pointLight1;
	Light* pointLight2;

	// Game entities
	Player* player;
	Ghost* ghostActor;

	// Systems
	FMODAudioSystem audioSystem;
	SceneData* sceneData;

	// Voronoi
	unique_ptr<Voronoi::VoronoiIslands> voronoiIslands;

	float SCREEN_WIDTH = 0.f;
	float SCREEN_HEIGHT = 0.f;

	int shadowmapWidth = 1024;
	int shadowmapHeight = 1024;
	int sceneWidth = 100;
	int sceneHeight = 100;
};

#endif