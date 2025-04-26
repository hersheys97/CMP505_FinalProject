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
#include "DepthShader.h"
#include "FireflyShader.h"
#include "WaterDepthShader.h"
#include "TerrainDepthShader.h"
#include "SceneData.h"

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

	// Rendering methods
	void getShadowDepthMap(const XMMATRIX& worldMatrix, const XMMATRIX& viewMatrix, const XMMATRIX& projectionMatrix, const XMMATRIX& identityMatrix);
	void finalRender(const XMMATRIX& worldMatrix, const XMMATRIX& viewMatrix, const XMMATRIX& projectionMatrix, const XMMATRIX& identityMatrix);

	void applyBloom(const XMMATRIX& worldMatrix, const XMMATRIX& viewMatrix, const XMMATRIX& projectionMatrix);
	void renderDome(const XMMATRIX& worldMatrix, const XMMATRIX& viewMatrix, const XMMATRIX& projectionMatrix);
	void renderTerrain(const XMMATRIX& worldMatrix, const XMMATRIX& viewMatrix, const XMMATRIX& projectionMatrix);
	void renderWater(const XMMATRIX& worldMatrix, const XMMATRIX& viewMatrix, const XMMATRIX& projectionMatrix);
	void renderMoon(const XMMATRIX& worldMatrix, const XMMATRIX& viewMatrix, const XMMATRIX& projectionMatrix);
	void renderFirefly(const XMMATRIX& worldMatrix, const XMMATRIX& viewMatrix, const XMMATRIX& projectionMatrix);

	// Cleanup method
	void cleanup();

	HWND hwnd;

	// Shadows
	ShadowMap* shadowMap[2];
	DepthShader* depthShader;
	WaterDepthShader* waterDepthShader;
	TerrainDepthShader* terrainDepthShader;

	// Render Texture
	RenderTexture* renderBloom;
	Bloom* bloomShader;

	// Dome
	SphereMesh* circleDome;
	DomeShader* domeShader;

	// Water
	PlaneMesh* water;
	WaterShader* waterShader;

	// Terrain
	PlaneMesh* topTerrain;
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