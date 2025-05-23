/*

SceneData.h

This header file is used to store all the data into a single struct for ease of access. It also has some toggle functions used in the GUI to trigger events.

*/

#pragma once
#include <DirectXMath.h>

// Forward declarations
class Light;
class Islands;
class Player;

// Light Data Structure
struct LightData {
public:
	float ambientColour[4] = { 1.f, 1.f, 1.f, 1.0f };
	float diffuseColour[4] = { 1.f, 1.f, 1.f, 1.0f };
	float specularColour[4] = { 0.8f, 0.9f, 1.0f, 1.0f };
	float spec_pow = 32.f;

	float pointLight_pos1[3] = { 56.8f, 16.25f, 78.75f };
	float pointLight1Colour[4] = { 1.f, 1.f, 1.f, 1.0f };
	float pointLight_pos2[3] = { 53.75f, 3.75f, 66.25f };
	float pointLight2Colour[4] = { 1.f, 1.f, 1.f, 1.0f };
	float pointLightRadius[2] = { 5.f, 5.f };
};

// Water Data Structure
struct WaterData {
public:
	float amplitude = 0.5f;
	float frequency = 0.37f;
	float speed = 2.f;
	float timeVal = 0.f;
};

// Shadow Lights Data Structure
struct ShadowLightsData {
public:
	float lightDirections[2][3] = { {0.056f, -0.722f, -1.0f}, {-0.045f, -1.f, -0.059f} };
	float dir_pos[3] = { 58.0f, 88.0f, 51.0f };
	float dirColour[4] = { 0.f, 0.023f, 0.035f, 1.0f };
	float spotColour[4] = { 0.1f, 0.15f, 0.25f, 1.0f };
	float spotCutoff = 0.988f;
	float spotFalloff = 5.0f;
	bool enableSpotShadow = true; // Toggle for spotlight shadows
	bool enableDirShadow = true; // Toggle for directional light shadows
};

// Moon Data Structure
struct MoonData {
public:
	float moon_pos[3] = { 58.611f, 66.0f, 134.0f };
};

// Bloom Data Structure
struct BloomData {
public:
	float blurAmount = 2.f;
	float blurIntensity = 2.f;
};

// Chromatic Aberration structure
struct ChromaticAberrationData {
	bool enabled = false;
	float intensity = 0.0f;
	float maxIntensity = 0.01f;
	float effectIntensity = 0.0f;
	float timeCalc = 0.0f;
	float ghostDistance = 0.0f;
	float effectFadeTimer = 0.0f;
	float effectFadeDuration = 1.0f;
	bool effectFadingIn = false;
	bool effectFadingOut = false;
	XMFLOAT2 offsets = { 0.0f, 0.0f };
	XMFLOAT2 ghostScreenPos = { 0.0f, 0.0f };
};

// Ghost State structure
struct GhostData {
	XMFLOAT3 position = { 0.f, 0.f, 0.f };
	XMFLOAT3 velocity = { 0.f, 0.f, 0.f };
	bool isActive = false;
	int currentIslandIndex = -1;
	float directionChangeTimer = 0.0f;
	float nextDirectionChangeTime = 7.0f;
	float aliveTime = 30.f;

	// Sonar response
	XMFLOAT3 sonarTargetPosition = { 0.f, 0.f, 0.f };
	float sonarResponseTimer = 0.f;
	bool respondingToSonar = false;
	float maxLifetime = 30.f;
};

struct SonarData {
	bool isActive = false;
	float sonarTime = 0.f;
	float sonarDuration = 5.f;
	XMFLOAT3 sonarOrigin = { 0.f, 0.f, 0.f };
};

// Player State structure
struct PlayerData {
	XMFLOAT3 lastCameraPosition = { 0.f, 0.f, 0.f };
	XMFLOAT3 cameraVelocity = { 0.f, 0.f, 0.f };
	float cameraEyeHeight = 1.8f;
	bool firstTimeInPlayMode = true;
};

// Audio State structure
struct AudioState {
	bool bgmStarted = false;
	float sonarMaxRadius = 100.0f;
};

// Scene Data Structure
struct SceneData {
public:
	LightData lightData;
	WaterData waterData;
	ShadowLightsData shadowLightsData;
	MoonData moonData;
	BloomData bloomData;
	ChromaticAberrationData chromaticAberrationData;
	GhostData ghostData;
	SonarData sonarData;
	PlayerData playerData;
	AudioState audioState;

	bool isGamePaused = true;

	float islandSize = 50.0f;
	int islandCount = 2;
	float minIslandDistance = 30.0f;
	int gridSize = 700;
	bool firstTimeGeneratingIslands = true;

	bool tessMesh = false;

	// Toggle spotlight shadow
	void toggleSpotShadow() {
		shadowLightsData.enableSpotShadow = !shadowLightsData.enableSpotShadow;
	}

	// Toggle directional light shadow
	void toggleDirShadow() {
		shadowLightsData.enableDirShadow = !shadowLightsData.enableDirShadow;
	}

	// Toggle tessellation - only show tessellated meshes
	void toggleTessellation() {
		tessMesh = !tessMesh;
	}

	// Reset the entire view to its initial state
	void resetView() {
		// Reset light data
		lightData.ambientColour[0] = 1.f; lightData.ambientColour[1] = 1.f; lightData.ambientColour[2] = 1.f; lightData.ambientColour[3] = 1.0f;
		lightData.diffuseColour[0] = 1.f; lightData.diffuseColour[1] = 1.f; lightData.diffuseColour[2] = 1.f; lightData.diffuseColour[3] = 1.0f;
		lightData.specularColour[0] = 0.8f; lightData.specularColour[1] = 0.9f; lightData.specularColour[2] = 1.0f; lightData.specularColour[3] = 1.0f;
		lightData.spec_pow = 32.f;
		lightData.pointLight_pos1[0] = 56.8f; lightData.pointLight_pos1[1] = 16.25f; lightData.pointLight_pos1[2] = 78.75f;
		lightData.pointLight1Colour[0] = 1.f; lightData.pointLight1Colour[1] = 1.f; lightData.pointLight1Colour[2] = 1.f; lightData.pointLight1Colour[3] = 1.0f;
		lightData.pointLight_pos2[0] = 53.75f; lightData.pointLight_pos2[1] = 3.75f; lightData.pointLight_pos2[2] = 66.25f;
		lightData.pointLight2Colour[0] = 1.f; lightData.pointLight2Colour[1] = 1.f; lightData.pointLight2Colour[2] = 1.f; lightData.pointLight2Colour[3] = 1.0f;
		lightData.pointLightRadius[0] = 5.f; lightData.pointLightRadius[1] = 5.f;

		// Reset water data
		waterData.amplitude = 0.5f;
		waterData.frequency = 0.37f;
		waterData.speed = 2.f;
		waterData.timeVal = 0.f;

		// Reset shadow lights data
		shadowLightsData.lightDirections[0][0] = 0.056f; shadowLightsData.lightDirections[0][1] = -0.722f; shadowLightsData.lightDirections[0][2] = -1.0f;
		shadowLightsData.lightDirections[1][0] = -0.045f; shadowLightsData.lightDirections[1][1] = -1.f; shadowLightsData.lightDirections[1][2] = -0.059f;
		shadowLightsData.dir_pos[0] = 58.0f; shadowLightsData.dir_pos[1] = 88.0f; shadowLightsData.dir_pos[2] = 51.0f;
		shadowLightsData.dirColour[0] = 0.f; shadowLightsData.dirColour[1] = 0.023f; shadowLightsData.dirColour[2] = 0.035f; shadowLightsData.dirColour[3] = 1.0f;
		shadowLightsData.spotColour[0] = 0.1f; shadowLightsData.spotColour[1] = 0.15f; shadowLightsData.spotColour[2] = 0.25f; shadowLightsData.spotColour[3] = 1.0f;
		shadowLightsData.spotCutoff = 0.988f;
		shadowLightsData.spotFalloff = 5.0f;
		shadowLightsData.enableSpotShadow = true;
		shadowLightsData.enableDirShadow = true;

		// Reset moon data
		moonData.moon_pos[0] = 58.611f; moonData.moon_pos[1] = 66.0f; moonData.moon_pos[2] = 134.0f;

		// Reset bloom data
		bloomData.blurAmount = 2.f;
		bloomData.blurIntensity = 2.f;

		// Reset class data
		ghostData = GhostData{};
		bloomData = BloomData{};
		chromaticAberrationData = ChromaticAberrationData{};
		audioState = AudioState{};
	}

	// Set the point light which will follow the player
	void setPointLight1Position(float x, float y, float z) {
		lightData.pointLight_pos1[0] = x;
		lightData.pointLight_pos1[1] = y;
		lightData.pointLight_pos1[2] = z;
	}

	// Set the ghost position
	void setGhostPosition(float x, float y, float z) {
		ghostData.position = XMFLOAT3(x, y, z);
	}

	// Chromatic Aberration effect
	void updateChromaticIntensity(float intensity) {
		chromaticAberrationData.intensity = intensity;
	}
};

// Pointer to SceneData
extern SceneData* sceneData;