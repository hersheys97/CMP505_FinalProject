#include "App1.h"
#include <DirectXMath.h>
#include <windows.h>
#include <string>
using namespace DirectX;

AppMode currentMode = AppMode::FlyCam;

float App1::randomFloat(float min, float max) {
	return min + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / (max - min)));
}

App1::App1() {
	circleDome = nullptr;
	domeShader = nullptr;
	renderBloom = nullptr;
	bloomShader = nullptr;
	water = nullptr;
	waterShader = nullptr;
	topTerrain = nullptr;
	terrainShader = nullptr;
	moon = nullptr;
	moonShader = nullptr;
	spotLight = nullptr;
	pointLight1 = nullptr;
	pointLight2 = nullptr;
	ghost = nullptr;
	depthShader = nullptr;
	waterDepthShader = nullptr;
	terrainDepthShader = nullptr;
	ghostShader = nullptr;
	sceneData = nullptr;

	for (int i = 0; i < 2; i++) {
		shadowMap[i] = nullptr;
	}
}

App1::~App1() { cleanup(); }

void App1::init(HINSTANCE hinstance, HWND hwnd, int screenWidth, int screenHeight, Input* in, bool VSYNC, bool FULL_SCREEN)
{
	BaseApplication::init(hinstance, hwnd, screenWidth, screenHeight, in, VSYNC, FULL_SCREEN);

	SCREEN_WIDTH = screenWidth;
	SCREEN_HEIGHT = screenHeight;

	initComponents();
}

bool App1::render() {

	// Clear back buffer once at start
	renderer->beginScene(0.f, 0.f, 0.f, 1.0f);

	// First pass: Render scene to render texture
	renderToTexture();

	// Second pass: Apply post-processing effects
	finalRenderToScreen();

	gui();
	renderer->endScene();
	return true;
}

/*****************************    Renders    ************************************/

void App1::renderToTexture() {

	// Set the render target to our aberration texture
	renderAberration->setRenderTarget(renderer->getDeviceContext());
	renderAberration->clearRenderTarget(renderer->getDeviceContext(), 0.0f, 1.0f, 0.0f, 1.0f);

	renderer->setAlphaBlending(true);

	// Set up the lights from Scene Data
	XMFLOAT3 playerPos = player->getPosition();

	// Update point light to player's position
	sceneData->setPointLight1Position(playerPos.x, playerPos.y + 10.f, playerPos.z);

	spotLight->setDiffuseColour(sceneData->lightData.diffuseColour[0], sceneData->lightData.diffuseColour[1], sceneData->lightData.diffuseColour[2], sceneData->lightData.diffuseColour[3]);
	spotLight->setDirection(sceneData->shadowLightsData.lightDirections[0][0], sceneData->shadowLightsData.lightDirections[0][1], sceneData->shadowLightsData.lightDirections[0][2]);
	spotLight->setPosition(sceneData->moonData.moon_pos[0], sceneData->moonData.moon_pos[1], sceneData->moonData.moon_pos[2]);

	directionalLight->setDirection(sceneData->shadowLightsData.lightDirections[1][0], sceneData->shadowLightsData.lightDirections[1][1], sceneData->shadowLightsData.lightDirections[1][2]);
	directionalLight->setPosition(sceneData->shadowLightsData.dir_pos[0], sceneData->shadowLightsData.dir_pos[1], sceneData->shadowLightsData.dir_pos[2]);

	pointLight1->setPosition(sceneData->lightData.pointLight_pos1[0], sceneData->lightData.pointLight_pos1[1], sceneData->lightData.pointLight_pos1[2]);
	pointLight1->setDiffuseColour(sceneData->lightData.pointLight1Colour[0], sceneData->lightData.pointLight1Colour[1], sceneData->lightData.pointLight1Colour[2], sceneData->lightData.pointLight1Colour[3]);

	pointLight2->setPosition(sceneData->lightData.pointLight_pos2[0], sceneData->lightData.pointLight_pos2[1], sceneData->lightData.pointLight_pos2[2]);
	pointLight2->setDiffuseColour(sceneData->lightData.pointLight2Colour[0], sceneData->lightData.pointLight2Colour[1], sceneData->lightData.pointLight2Colour[2], sceneData->lightData.pointLight2Colour[3]);

	sceneData->waterData.timeVal += timer->getTime();

	camera->update();

	XMMATRIX worldMatrix = renderer->getWorldMatrix();
	XMMATRIX viewMatrix = camera->getViewMatrix();
	XMMATRIX projectionMatrix = renderer->getProjectionMatrix();
	XMMATRIX identity = XMMatrixIdentity();

	renderAudio();
	getShadowDepthMap(worldMatrix, viewMatrix, projectionMatrix, identity);
	finalRender(worldMatrix, viewMatrix, projectionMatrix, identity);

	// Reset to back buffer
	renderer->setBackBufferRenderTarget();
}

void App1::finalRenderToScreen() {
	XMMATRIX worldMatrix = XMMatrixIdentity();
	XMMATRIX orthoMatrix = renderer->getOrthoMatrix();
	XMMATRIX orthoViewMatrix = camera->getOrthoViewMatrix();

	renderer->setZBuffer(false);
	renderer->setWireframeMode(false);
	renderer->setCullOn(false);

	screenEffects->sendData(renderer->getDeviceContext());

	if (sceneData->chromaticAberrationData.enabled) applyChromaticAberration(worldMatrix, orthoViewMatrix, orthoMatrix);
	else {
		simpleTexture->setShaderParameters(renderer->getDeviceContext(), worldMatrix, orthoViewMatrix, orthoMatrix, renderAberration->getShaderResourceView());
		simpleTexture->render(renderer->getDeviceContext(), screenEffects->getIndexCount());
	}
	renderer->setZBuffer(true);
}


void App1::renderAudio() {
	audioSystem.update(timer->getTime());

	// Update listener position
	XMFLOAT3 camPos = camera->getPosition();
	XMFLOAT3 camForward = camera->getForward();
	XMFLOAT3 camUp = camera->getUp();
	audioSystem.updateListenerPosition(camPos, camForward, camUp);

	// Update island ambiences - always pass the closest island
	if (voronoiIslands) {
		int closestIsland = voronoiIslands->GetClosestIslandIndex(camPos);
		if (closestIsland >= 0) audioSystem.updateIslandAmbiences(camPos, closestIsland);
	}
}

void App1::renderPlayer() {
	float dt = timer->getTime();
	if (dt < 1e-6f) dt = 0.016f;

	if (currentMode == AppMode::Play) {

		ShowCursor(FALSE);

		if (input->isKeyDown(VK_ESCAPE)) {
			camera->setPosition(40.8f, 13.f, -11.8f);
			camera->setRotation(2.3f, 9.9f, 0.f);
			currentMode = AppMode::FlyCam;
			input->SetKeyUp(VK_ESCAPE); // Mark as handled
			ShowCursor(TRUE);
			return;
		}

		player->updatePlayer(dt, input, terrainShader, camera, &audioSystem, voronoiIslands.get());
		player->handleMouseLook(input, dt, hwnd, sceneWidth, sceneHeight);
		player->handlePlayModeReset(voronoiIslands.get(), terrainShader, camera);
		player->handleSonar(input, &audioSystem);
		audioSystem.playGhostWhisper(sceneData->ghostData.position);
		audioSystem.playBGM1();
	}
	else {
		ShowCursor(TRUE);

		if (sceneData->audioState.bgmStarted) {
			audioSystem.stopBGM1();
			sceneData->audioState.bgmStarted = false;
		}
	}

	// Handle sonar timer
	if (sceneData->sonarData.isActive) {
		sceneData->sonarData.sonarTime += timer->getTime();
		if (sceneData->sonarData.sonarTime >= sceneData->sonarData.sonarDuration) {
			sceneData->sonarData.isActive = false;
			sceneData->tessMesh = false;
			wireframeToggle = false;
		}
	}
}

void App1::renderGhost(const XMMATRIX& worldMatrix, const XMMATRIX& viewMatrix, const XMMATRIX& projectionMatrix) {
	float deltaTime = timer->getTime();

	if (!sceneData->ghostData.respondingToSonar) sceneData->ghostData.aliveTime -= deltaTime;

	if (!sceneData->ghostData.isActive || sceneData->ghostData.maxLifetime <= 0.f) handleGhostRespawn();
	else {
		if (sceneData->ghostData.respondingToSonar) updateSonarResponse(deltaTime);
		else handleNormalWandering(deltaTime);

		updateGhostPosition(deltaTime);
		updateChromaticAberration();
	}

	if (sceneData->ghostData.isActive) {
		renderGhostModel(worldMatrix, viewMatrix, projectionMatrix, deltaTime);
		updateGhostAudio(deltaTime);
	}

	if (!sceneData->ghostData.isActive && audioSystem.isWhisperPlaying()) audioSystem.stopGhostWhisper();
}

void App1::handleGhostRespawn() {
	if (audioSystem.isWhisperPlaying()) {
		audioSystem.stopGhostWhisper();
	}

	if (!sceneData->ghostData.respondingToSonar) {
		const auto& islands = voronoiIslands->GetIslands();
		if (!islands.empty()) {
			sceneData->ghostData.currentIslandIndex = rand() % islands.size();
			const auto& island = islands[sceneData->ghostData.currentIslandIndex];

			sceneData->ghostData.position = XMFLOAT3{ island.position.x + (rand() % 10 - 5),island.position.y + 3.f,island.position.z + (rand() % 10 - 5) };
			sceneData->ghostData.velocity = XMFLOAT3{ randomFloat(-1.f, 1.f) * 2.f,0.f,randomFloat(-1.f, 1.f) * 2.f };
			sceneData->ghostData.aliveTime = sceneData->ghostData.maxLifetime;
			sceneData->ghostData.isActive = true;

			sceneData->ghostData.directionChangeTimer = 0.0f;
			sceneData->ghostData.nextDirectionChangeTime = randomFloat(1.0f, 2.0f);
		}
	}
}

void App1::updateSonarResponse(float deltaTime) {
	if (!sceneData->ghostData.respondingToSonar) return;

	sceneData->ghostData.sonarResponseTimer += deltaTime;

	XMVECTOR targetPos = XMLoadFloat3(&sceneData->ghostData.sonarTargetPosition);
	XMVECTOR currentPos = XMLoadFloat3(&sceneData->ghostData.position);
	XMVECTOR toTarget = XMVectorSubtract(targetPos, currentPos);
	float distance = XMVectorGetX(XMVector3Length(toTarget));

	// If ghost has reached target or sonar duration expired
	if (distance < 0.5f || sceneData->ghostData.sonarResponseTimer >= sceneData->sonarData.sonarDuration) {
		// Store the ghost's state before sonar response
		static XMFLOAT3 preSonarVelocity;
		static float preSonarDirectionChangeTimer;
		static float preSonarNextDirectionChangeTime;

		if (distance < 0.5f) { // Only reached target
			// Save current state before changing
			preSonarVelocity = sceneData->ghostData.velocity;
			preSonarDirectionChangeTimer = sceneData->ghostData.directionChangeTimer;
			preSonarNextDirectionChangeTime = sceneData->ghostData.nextDirectionChangeTime;
		}

		// Reset sonar response state
		sceneData->ghostData.respondingToSonar = false;
		sceneData->ghostData.sonarResponseTimer = 0.0f;

		// Respawn ghost to a random island position
		const auto& islands = voronoiIslands->GetIslands();
		if (!islands.empty()) {
			sceneData->ghostData.currentIslandIndex = rand() % islands.size();
			const auto& island = islands[sceneData->ghostData.currentIslandIndex];

			// Set new position but restore previous movement behavior
			sceneData->ghostData.position = XMFLOAT3{
				island.position.x + (rand() % 10 - 5),
				island.position.y + 3.f,
				island.position.z + (rand() % 10 - 5)
			};

			// Restore previous velocity and wandering settings
			sceneData->ghostData.velocity = preSonarVelocity;
			sceneData->ghostData.directionChangeTimer = preSonarDirectionChangeTimer;
			sceneData->ghostData.nextDirectionChangeTime = preSonarNextDirectionChangeTime;

			// Reset wandering timer to give immediate new direction
			sceneData->ghostData.directionChangeTimer = 0.0f;
			sceneData->ghostData.nextDirectionChangeTime = 0.0f;
		}
	}
	else {
		// Continue moving toward target
		float remainingTime = sceneData->sonarData.sonarDuration - sceneData->ghostData.sonarResponseTimer;
		float requiredSpeed = (remainingTime > 0) ? (distance / remainingTime) : 0.f;

		XMVECTOR direction = XMVector3Normalize(toTarget);
		XMStoreFloat3(&sceneData->ghostData.velocity, XMVectorScale(direction, requiredSpeed));
	}
}

void App1::handleNormalWandering(float deltaTime) {
	const auto& island = voronoiIslands->GetIslands()[sceneData->ghostData.currentIslandIndex];
	float halfSize = 25.0f;
	float minX = island.position.x - halfSize;
	float maxX = island.position.x + halfSize;
	float minZ = island.position.z - halfSize;
	float maxZ = island.position.z + halfSize;

	bool bounced = handleBoundaryBounce(minX, maxX, minZ, maxZ);
	if (bounced) {
		sceneData->ghostData.directionChangeTimer = 0.0f;
		sceneData->ghostData.nextDirectionChangeTime = randomFloat(5.0f, 10.0f);
	}

	sceneData->ghostData.directionChangeTimer += deltaTime;
	if (sceneData->ghostData.directionChangeTimer >= sceneData->ghostData.nextDirectionChangeTime) {
		updateWanderingDirection();
		sceneData->ghostData.directionChangeTimer = 0.0f;
		sceneData->ghostData.nextDirectionChangeTime = randomFloat(5.0f, 10.0f);
	}
}

bool App1::handleBoundaryBounce(float minX, float maxX, float minZ, float maxZ) {
	bool bounced = false;

	if (sceneData->ghostData.position.x < minX || sceneData->ghostData.position.x > maxX) {
		sceneData->ghostData.velocity.x *= -1.f;
		sceneData->ghostData.position.x = max(minX, min(maxX, sceneData->ghostData.position.x));
		bounced = true;
	}
	if (sceneData->ghostData.position.z < minZ || sceneData->ghostData.position.z > maxZ) {
		sceneData->ghostData.velocity.z *= -1.f;
		sceneData->ghostData.position.z = max(minZ, min(maxZ, sceneData->ghostData.position.z));
		bounced = true;
	}

	return bounced;
}

void App1::updateWanderingDirection() {
	sceneData->ghostData.velocity = XMFLOAT3{ randomFloat(5.f, 8.f), 0.f, randomFloat(5.f, 8.f) };
}

void App1::updateGhostPosition(float deltaTime) {
	sceneData->ghostData.position.x += sceneData->ghostData.velocity.x * deltaTime;
	sceneData->ghostData.position.z += sceneData->ghostData.velocity.z * deltaTime;
}

void App1::updateChromaticAberration() {
	XMFLOAT3 playerPos = player->getPosition();
	XMVECTOR ghostPos = XMLoadFloat3(&sceneData->ghostData.position);
	XMVECTOR playerPosVec = XMLoadFloat3(&playerPos);
	float distance = XMVectorGetX(XMVector3Length(XMVectorSubtract(ghostPos, playerPosVec)));

	if (distance <= 50.0f) {
		sceneData->chromaticAberrationData.enabled = true;
		sceneData->chromaticAberrationData.intensity = sceneData->chromaticAberrationData.maxIntensity * (1.0f - (distance / 50.0f));
	}
	else {
		sceneData->chromaticAberrationData.enabled = false;
		sceneData->chromaticAberrationData.intensity = 0.0f;
	}
}

void App1::renderGhostModel(const XMMATRIX& worldMatrix, const XMMATRIX& viewMatrix, const XMMATRIX& projectionMatrix, float deltaTime) {
	XMFLOAT3 ghostScreenPos;
	XMVECTOR ghostPos = XMLoadFloat3(&sceneData->ghostData.position);
	XMMATRIX viewProj = viewMatrix * projectionMatrix;
	XMVECTOR screenPos = XMVector3Project(ghostPos, 0, 0, sceneWidth, sceneHeight, 0.0f, 1.0f, projectionMatrix, viewMatrix, XMMatrixIdentity());
	XMStoreFloat3(&ghostScreenPos, screenPos);

	ghostScreenPos.x /= sceneWidth;
	ghostScreenPos.y /= sceneHeight;

	XMFLOAT3 camPos = camera->getPosition();
	XMVECTOR camPosVec = XMLoadFloat3(&camPos);
	XMVECTOR distanceVec = XMVector3Length(XMVectorSubtract(ghostPos, camPosVec));
	float maxDistance = 50.0f;
	sceneData->chromaticAberrationData.ghostDistance = 1.0f - min(1.0f, XMVectorGetX(distanceVec) / maxDistance);

	sceneData->chromaticAberrationData.timeCalc += deltaTime;
	sceneData->chromaticAberrationData.effectIntensity = sceneData->chromaticAberrationData.intensity;

	float offsetAngle = sceneData->chromaticAberrationData.timeCalc * 2.0f;
	float offsetMagnitude = sceneData->chromaticAberrationData.effectIntensity * 0.05f;
	sceneData->chromaticAberrationData.offsets.x = cos(offsetAngle) * offsetMagnitude;
	sceneData->chromaticAberrationData.offsets.y = sin(offsetAngle) * offsetMagnitude;

	XMMATRIX ghostWorldMatrix = XMMatrixTranslation(sceneData->ghostData.position.x, sceneData->ghostData.position.y, sceneData->ghostData.position.z) * worldMatrix;
	ghost->sendData(renderer->getDeviceContext());
	ghostShader->setShaderParameters(renderer->getDeviceContext(), ghostWorldMatrix, viewMatrix, projectionMatrix, textureMgr->getTexture(L"ghost"), camera, spotLight, directionalLight, sceneData);
	ghostShader->render(renderer->getDeviceContext(), ghost->getIndexCount());
}

void App1::updateGhostAudio(float deltaTime) {
	audioSystem.updateGhostPosition(sceneData->ghostData.position);
	audioSystem.updateGhostWhisperVolume(camera->getPosition());
	audioSystem.updateGhostEffects(deltaTime, camera->getPosition());
}


// Shadow Depth Map
// Two types of lights are used for shadow depth mapping: a directional light and a spotlight. The shadow map is set up for rendering by binding the depth buffer and disabling colour rendering. Shadows are created by rendering the scene from the perspective of each light, capturing depth information. This data is then used in the final render to produce shadows. The position is calculated and updated in the lookAt function, which is used to generate the orthographic matrix. After generating the view matrix for both lights and rendering the meshes with their respective shaders, the render target is reset to the back buffer, and the viewport is restored.

void App1::getShadowDepthMap(const XMMATRIX& worldMatrix, const XMMATRIX& viewMatrix, const XMMATRIX& projectionMatrix, const XMMATRIX& identityMatrix) {

	Light* lights[2] = { spotLight, directionalLight };

	// Repeat for each light source
	for (int i = 0; i < 2; ++i) {

		Light* currentLight = lights[i];

		// 1. Set Shadow Map as Render Target
		shadowMap[i]->BindDsvAndSetNullRenderTarget(renderer->getDeviceContext());

		// Set up light matrices
		XMFLOAT3 lightPos = currentLight->getPosition();
		XMFLOAT3 lightDir = currentLight->getDirection();

		// Calculate target position properly
		XMFLOAT3 targetPos = {
			lightPos.x + lightDir.x * 10.0f, // Multiply by some distance
			lightPos.y + lightDir.y * 10.0f,
			lightPos.z + lightDir.z * 10.0f
		};

		currentLight->setLookAt(targetPos.x, targetPos.y, targetPos.z);
		currentLight->generateViewMatrix();

		// 2. Get matrix based on light's position and direction
		currentLight->generateViewMatrix();

		XMMATRIX lightViewMatrix = currentLight->getViewMatrix();
		XMMATRIX lightProjectionMatrix = currentLight->getOrthoMatrix();

		// 3. Apply Depth Shader to the meshes

		// Islands, bridges, pickups
		generateIslands(worldMatrix, viewMatrix, projectionMatrix, true, lightViewMatrix, lightProjectionMatrix);
		generateBridges(worldMatrix, viewMatrix, projectionMatrix, true, lightViewMatrix, lightProjectionMatrix);
		generatePickups(worldMatrix, viewMatrix, projectionMatrix, true, lightViewMatrix, lightProjectionMatrix);

		// Ghost
		XMMATRIX ghostWorldMatrix = XMMatrixTranslation(sceneData->ghostData.position.x, sceneData->ghostData.position.y, sceneData->ghostData.position.z);
		ghost->sendData(renderer->getDeviceContext());
		depthShader->setShaderParameters(renderer->getDeviceContext(), ghostWorldMatrix, lightViewMatrix, lightProjectionMatrix);
		depthShader->render(renderer->getDeviceContext(), ghost->getIndexCount());

		// Water - own vertex shader needed to calculate displacements to cast shows correclty on it
		//XMMATRIX waterWorldMatrix = XMMatrixTranslation(1.f, 0.0f, 1.0f) * worldMatrix;
		//water->sendData(renderer->getDeviceContext());
		//waterDepthShader->setShaderParameters(renderer->getDeviceContext(), waterWorldMatrix, viewMatrix, projectionMatrix, sceneData);
		//waterDepthShader->render(renderer->getDeviceContext(), water->getIndexCount());
	}

	// Reset render target and viewport after rendering for this light
	renderer->setBackBufferRenderTarget();
	renderer->resetViewport();
}

// Final Render
void App1::finalRender(const XMMATRIX& worldMatrix, const XMMATRIX& viewMatrix, const XMMATRIX& projectionMatrix, const XMMATRIX& identityMatrix)
{
	//A conditional check ensures that tessellation is only applied if it has been enabled in the GUI. If enabled, tessellated meshes are loaded for rendering. Also, wireframe is enabled.
	if (sceneData->tessMesh)
	{
		wireframeToggle = true;
		renderTerrain(worldMatrix, viewMatrix, projectionMatrix);
	}
	else if (!sceneData->tessMesh)
	{
		// Post-processing effect: Bloom on stars texture along with Gaussian blur
		applyBloom(worldMatrix, identityMatrix, projectionMatrix);

		renderTerrain(worldMatrix, viewMatrix, projectionMatrix);
		renderDome(worldMatrix, viewMatrix, projectionMatrix);
		//renderWater(worldMatrix, viewMatrix, projectionMatrix);
		renderMoon(worldMatrix, viewMatrix, projectionMatrix);
	}

	renderPlayer();
	renderGhost(worldMatrix, viewMatrix, projectionMatrix);

	// Reset to back buffer
	renderer->setBackBufferRenderTarget();
}

void App1::applyChromaticAberration(const XMMATRIX& worldMatrix, const XMMATRIX& orthoViewMatrix, const XMMATRIX& orthoMatrix)
{
	chromaticAberration->setShaderParameters(renderer->getDeviceContext(), worldMatrix, orthoViewMatrix, orthoMatrix, renderAberration->getShaderResourceView(), sceneData);
	chromaticAberration->render(renderer->getDeviceContext(), screenEffects->getIndexCount());
}

void App1::applyBloom(const XMMATRIX& worldMatrix, const XMMATRIX& viewMatrix, const XMMATRIX& projectionMatrix) {
	renderBloom->setRenderTarget(renderer->getDeviceContext());
	renderBloom->clearRenderTarget(renderer->getDeviceContext(), 0.0f, 0.0f, 0.0f, 1.0f);

	camera->update();

	XMMATRIX skyDomeWorldMatrix = XMMatrixScaling(200.f, 200.f, 200.f) * XMMatrixTranslation(50.f, 30.f, 50.f) * worldMatrix;

	circleDome->sendData(renderer->getDeviceContext());
	bloomShader->setShaderParameters(renderer->getDeviceContext(), skyDomeWorldMatrix, viewMatrix, projectionMatrix, textureMgr->getTexture(L"dome"), SCREEN_WIDTH, SCREEN_HEIGHT, sceneData);
	bloomShader->render(renderer->getDeviceContext(), circleDome->getIndexCount());

	renderer->setBackBufferRenderTarget();
}

void App1::renderDome(const XMMATRIX& worldMatrix, const XMMATRIX& viewMatrix, const XMMATRIX& projectionMatrix) {

	XMMATRIX skyDomeWorldMatrix = XMMatrixScaling(1000.f, 200.f, 1000.f) * XMMatrixTranslation(50.f, 30.f, 50.f) * worldMatrix;

	circleDome->sendData(renderer->getDeviceContext());
	domeShader->setShaderParameters(renderer->getDeviceContext(), skyDomeWorldMatrix, viewMatrix, projectionMatrix, renderBloom->getShaderResourceView(), sceneData);
	domeShader->render(renderer->getDeviceContext(), circleDome->getIndexCount());
}

// Procedural Generation of Voronoi Islands
// Based on the number of islands, that many Voronoi regions are created in voronoiIslands.GenerateVoronoiRegions(). Then, inside each Voronoi region, an island is spawn with a random position and rotation. After that, each island connects to one other island with a bridge. The islands have collision detection with the Player (Play Mode) or the Camera (Fly Mode).

void App1::generateIslands(const XMMATRIX& worldMatrix, const XMMATRIX& viewMatrix, const XMMATRIX& projectionMatrix, bool depth, const XMMATRIX& lightViewMatrix, const XMMATRIX& lightProjectionMatrix) {
	constexpr float ISLAND_SCALE = 50.0f;
	auto& islands = voronoiIslands->GetIslands();

	// Clear existing ambiences only on first generation
	if (sceneData->firstTimeGeneratingIslands) {
		audioSystem.stopAllIslandAmbience();
		sceneData->firstTimeGeneratingIslands = false;

		// Create ambiences for all islands at once
		for (auto& island : islands) {
			if (!island.initialized) continue;
			float height = terrainShader->getHeight(island.position.x, island.position.z);
			audioSystem.createIslandAmbience(XMFLOAT3(island.position.x, height, island.position.z));
		}
	}

	for (auto& island : islands) {
		if (!island.initialized) continue;

		float height = terrainShader->getHeight(island.position.x, island.position.z);
		XMMATRIX islandWorld = XMMatrixScaling(ISLAND_SCALE, 1.0f, ISLAND_SCALE) * XMMatrixRotationY(island.rotationY) * XMMatrixTranslation(island.position.x, height, island.position.z) * worldMatrix;

		if (depth) {
			topTerrain->sendData(renderer->getDeviceContext(), D3D_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST);
			terrainDepthShader->setShaderParameters(renderer->getDeviceContext(), islandWorld, lightViewMatrix, lightProjectionMatrix, textureMgr->getTexture(L"island_floor"), textureMgr->getTexture(L""), camera);
			terrainDepthShader->render(renderer->getDeviceContext(), topTerrain->getIndexCount());
		}
		else {
			topTerrain->sendData(renderer->getDeviceContext(), D3D_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST);
			terrainShader->setShaderParameters(renderer->getDeviceContext(), islandWorld, viewMatrix, projectionMatrix,
				sceneData->audioState.sonarMaxRadius * (sceneData->sonarData.sonarTime / sceneData->sonarData.sonarDuration),
				textureMgr->getTexture(L"island_floor"),
				sceneData->shadowLightsData.enableSpotShadow ? shadowMap[0]->getDepthMapSRV() : nullptr,
				sceneData->shadowLightsData.enableDirShadow ? shadowMap[1]->getDepthMapSRV() : nullptr,
				camera, spotLight, directionalLight, sceneData
			);
			terrainShader->render(renderer->getDeviceContext(), topTerrain->getIndexCount());
		}
	}
}

void App1::generatePickups(const XMMATRIX& worldMatrix, const XMMATRIX& viewMatrix, const XMMATRIX& projectionMatrix, bool depth, const XMMATRIX& lightViewMatrix, const XMMATRIX& lightProjectionMatrix) {
	const float scaleTeapot = 0.2f;

	for (const auto& island : voronoiIslands->GetIslands()) {
		if (!island.initialized) continue;

		for (const auto& position : island.pickupPositions) {
			XMVECTOR spawnPos = XMLoadFloat3(&position);
			XMMATRIX teapotWorld = XMMatrixScaling(scaleTeapot, scaleTeapot, scaleTeapot) * XMMatrixTranslation(XMVectorGetX(spawnPos), XMVectorGetY(spawnPos) + 1.f, XMVectorGetZ(spawnPos)) * worldMatrix;

			if (depth) {
				teapot->sendData(renderer->getDeviceContext());
				depthShader->setShaderParameters(renderer->getDeviceContext(), teapotWorld, lightViewMatrix, lightProjectionMatrix);
				depthShader->render(renderer->getDeviceContext(), teapot->getIndexCount());
			}
			else {
				teapot->sendData(renderer->getDeviceContext());
				ghostShader->setShaderParameters(renderer->getDeviceContext(), teapotWorld, viewMatrix, projectionMatrix, textureMgr->getTexture(L"teapot"), camera, spotLight, directionalLight, sceneData);
				ghostShader->render(renderer->getDeviceContext(), teapot->getIndexCount());
			}
		}
	}
}

void App1::generateBridges(const XMMATRIX& worldMatrix, const XMMATRIX& viewMatrix, const XMMATRIX& projectionMatrix, bool depth, const XMMATRIX& lightViewMatrix, const XMMATRIX& lightProjectionMatrix)
{
	constexpr float BRIDGE_WIDTH = 5.0f;
	constexpr float BRIDGE_HEIGHT = 0.5f;
	constexpr float HALF_REGION_SIZE = 75.0f;
	const auto& islands = voronoiIslands->GetIslands();

	for (const auto& bridge : voronoiIslands->GetBridges())
	{
		const auto& islandA = islands[bridge.islandA];
		const auto& islandB = islands[bridge.islandB];

		// Get heights at bridge endpoints
		float heightA = terrainShader->getHeight(islandA.position.x, islandA.position.z);
		float heightB = terrainShader->getHeight(islandB.position.x, islandB.position.z);
		float avgHeight = (heightA + heightB) * 0.5f + BRIDGE_HEIGHT;

		// Calculate bridge direction and angle
		XMVECTOR posA = XMLoadFloat3(&islandA.position);
		XMVECTOR posB = XMLoadFloat3(&islandB.position);
		XMVECTOR dir = XMVector3Normalize(posB - posA);
		float angle = atan2(XMVectorGetZ(dir), XMVectorGetX(dir));

		// Calculate bridge endpoints
		XMFLOAT3 exitPointA = islandA.position;
		XMFLOAT3 entryPointB = islandB.position;

		if (fabs(XMVectorGetX(dir)) > fabs(XMVectorGetZ(dir))) {
			exitPointA.x += (XMVectorGetX(dir) > 0 ? HALF_REGION_SIZE : -HALF_REGION_SIZE);
			exitPointA.z += tan(angle) * HALF_REGION_SIZE;
			entryPointB.x += (XMVectorGetX(dir) > 0 ? -HALF_REGION_SIZE : HALF_REGION_SIZE);
			entryPointB.z += tan(angle) * -HALF_REGION_SIZE;
		}
		else {
			exitPointA.z += (XMVectorGetZ(dir) > 0 ? HALF_REGION_SIZE : -HALF_REGION_SIZE);
			exitPointA.x += 1.0f / tan(angle) * HALF_REGION_SIZE;
			entryPointB.z += (XMVectorGetZ(dir) > 0 ? -HALF_REGION_SIZE : HALF_REGION_SIZE);
			entryPointB.x += 1.0f / tan(angle) * -HALF_REGION_SIZE;
		}

		// Create bridge transform
		XMVECTOR bridgeStart = XMLoadFloat3(&exitPointA);
		XMVECTOR bridgeEnd = XMLoadFloat3(&entryPointB);
		XMVECTOR bridgeCenter = (bridgeStart + bridgeEnd) * 0.5f;
		float bridgeLength = XMVectorGetX(XMVector3Length(bridgeEnd - bridgeStart));

		XMMATRIX bridgeWorld = XMMatrixScaling(bridgeLength, BRIDGE_HEIGHT, BRIDGE_WIDTH) * XMMatrixRotationY(-angle) * XMMatrixTranslation(XMVectorGetX(bridgeCenter), avgHeight, XMVectorGetZ(bridgeCenter)) * worldMatrix;

		// Render bridge
		float sonarRadius = sceneData->audioState.sonarMaxRadius * (sceneData->sonarData.sonarTime / sceneData->sonarData.sonarDuration);

		if (depth) {
			topTerrain->sendData(renderer->getDeviceContext(), D3D_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST);
			terrainDepthShader->setShaderParameters(renderer->getDeviceContext(), bridgeWorld, lightViewMatrix, lightProjectionMatrix, textureMgr->getTexture(L"island_floor"), textureMgr->getTexture(L""), camera);
			terrainDepthShader->render(renderer->getDeviceContext(), topTerrain->getIndexCount());
		}
		topTerrain->sendData(renderer->getDeviceContext(), D3D_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST);
		terrainShader->setShaderParameters(renderer->getDeviceContext(), bridgeWorld, viewMatrix, projectionMatrix, sonarRadius, textureMgr->getTexture(L"island_floor"),
			sceneData->shadowLightsData.enableSpotShadow ? shadowMap[0]->getDepthMapSRV() : nullptr,
			sceneData->shadowLightsData.enableDirShadow ? shadowMap[1]->getDepthMapSRV() : nullptr,
			camera, spotLight, directionalLight, sceneData
		);
		terrainShader->render(renderer->getDeviceContext(), topTerrain->getIndexCount());
	}
}

void App1::renderTerrain(const XMMATRIX& worldMatrix, const XMMATRIX& viewMatrix, const XMMATRIX& projectionMatrix) {
	if (!wireframeToggle) renderer->setCullOn(false);

	generateIslands(worldMatrix, viewMatrix, projectionMatrix, false, viewMatrix, viewMatrix);
	generateBridges(worldMatrix, viewMatrix, projectionMatrix, false, viewMatrix, viewMatrix);
	generatePickups(worldMatrix, viewMatrix, projectionMatrix, false, viewMatrix, viewMatrix);

	if (!wireframeToggle)renderer->setCullOn(true);
}

void App1::renderWater(const XMMATRIX& worldMatrix, const XMMATRIX& viewMatrix, const XMMATRIX& projectionMatrix) {
	XMMATRIX waterWorldMatrix = XMMatrixScaling(700.f, 1.0f, 700.f) * XMMatrixTranslation(1.f, -50.f, 1.0f) * worldMatrix;

	if (!wireframeToggle) renderer->setCullBack(true);

	// GUI conditional statement - Toggle shadows
	water->sendData(renderer->getDeviceContext());
	waterShader->setShaderParameters(renderer->getDeviceContext(), waterWorldMatrix, viewMatrix, projectionMatrix, textureMgr->getTexture(L"water"), sceneData->shadowLightsData.enableSpotShadow ? shadowMap[0]->getDepthMapSRV() : nullptr, sceneData->shadowLightsData.enableDirShadow ? shadowMap[1]->getDepthMapSRV() : nullptr, camera, spotLight, directionalLight, sceneData);
	waterShader->render(renderer->getDeviceContext(), water->getIndexCount());

	if (!wireframeToggle) renderer->setCullBack(false);
}

void App1::renderMoon(const XMMATRIX& worldMatrix, const XMMATRIX& viewMatrix, const XMMATRIX& projectionMatrix) {
	XMMATRIX moonWorldMatrix = XMMatrixScaling(10.0f, 10.0f, 10.0f) * XMMatrixTranslation(sceneData->moonData.moon_pos[0], sceneData->moonData.moon_pos[1], sceneData->moonData.moon_pos[2]) * worldMatrix;

	moon->sendData(renderer->getDeviceContext());
	moonShader->setShaderParameters(renderer->getDeviceContext(), moonWorldMatrix, viewMatrix, projectionMatrix, textureMgr->getTexture(L"moon"), spotLight, sceneData);
	moonShader->render(renderer->getDeviceContext(), moon->getIndexCount());
}

/*****************************    GUI    ************************************/

void App1::gui()
{
	// Force turn off unnecessary shader stages.
	renderer->getDeviceContext()->GSSetShader(NULL, NULL, 0);
	renderer->getDeviceContext()->HSSetShader(NULL, NULL, 0);
	renderer->getDeviceContext()->DSSetShader(NULL, NULL, 0);

	ImGuiStyle& style = ImGui::GetStyle();
	style.Colors[ImGuiCol_WindowBg] = ImVec4(0, 0, 0, 0.4); // Fully transparent background

	// Window 1
	ImGui::SetNextWindowPos(ImVec2(SCREEN_WIDTH - 250, 10), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(250, 400), ImGuiCond_FirstUseEver);

	ImGui::Begin("Graphics Settings and Debugging", nullptr, ImGuiWindowFlags_None);

	if (ImGui::CollapsingHeader("Performance & Camera Info"))
	{
		ImGui::Text("FPS: %.2f", timer->getFPS());
		ImGui::Text("Time: %.2f", sceneData->waterData.timeVal);
		ImGui::Text("Camera Position: X = %.3f, Y = %.3f, Z = %.3f",
			camera->getPosition().x, camera->getPosition().y, camera->getPosition().z);
		ImGui::Text("Camera Rotation: X = %.3f, Y = %.3f, Z = %.3f",
			camera->getRotation().x, camera->getRotation().y, camera->getRotation().z);
		ImGui::Text("Player Position: X = %.3f, Y = %.3f, Z = %.3f",
			player->getPosition().x, player->getPosition().y, player->getPosition().z);
		ImGui::Text("PL1 Position: X = %.3f, Y = %.3f, Z = %.3f",
			pointLight1->getPosition().x, pointLight1->getPosition().y, pointLight1->getPosition().z);
	}
	if (ImGui::CollapsingHeader("Lighting Settings"))
	{
		ImGui::ColorEdit4("Ambient Colour", sceneData->lightData.ambientColour);
		ImGui::ColorEdit4("Diffuse Colour", sceneData->lightData.diffuseColour);
		ImGui::SliderFloat("Specular Power", &sceneData->lightData.spec_pow, 0.1f, 1000.f);
		ImGui::ColorEdit4("Specular Colour", sceneData->lightData.specularColour);

		ImGui::Separator();

		ImGui::Text("Point Light 1");
		ImGui::SliderFloat3("Position 1", sceneData->lightData.pointLight_pos1, 0.0f, 90.0);
		ImGui::SliderFloat("Radius 1", &sceneData->lightData.pointLightRadius[0], 0.f, 10.f);
		ImGui::ColorEdit4("Colour 1", sceneData->lightData.pointLight1Colour);

		ImGui::Separator();

		ImGui::Text("Point Light 2");
		ImGui::SliderFloat3("Position 2", sceneData->lightData.pointLight_pos2, 0.0f, 90.0);
		ImGui::SliderFloat("Radius 2", &sceneData->lightData.pointLightRadius[1], 0.f, 10.f);
		ImGui::ColorEdit4("Colour 2", sceneData->lightData.pointLight2Colour);

		ImGui::Separator();

		ImGui::Text("Moonlight (Spotlight)");

		ImGui::SliderFloat3("Position 3", sceneData->moonData.moon_pos, -15.f, 250.0f);
		ImGui::SliderFloat3("Spot Direction", sceneData->shadowLightsData.lightDirections[0], -1.0f, 1.0f);
		ImGui::ColorEdit4("Colour 3", sceneData->shadowLightsData.spotColour);
		ImGui::SliderFloat("Cutoff", &sceneData->shadowLightsData.spotCutoff, 0.f, 10.f);
		ImGui::SliderFloat("Falloff", &sceneData->shadowLightsData.spotFalloff, 0.f, 10.f);

		ImGui::Separator();

		ImGui::Text("Directional Light");

		ImGui::SliderFloat3("Direction", sceneData->shadowLightsData.lightDirections[1], -1.0f, 1.0f);
		ImGui::ColorEdit4("Colour 4", sceneData->shadowLightsData.dirColour);
		ImGui::SliderFloat3("Position 4", sceneData->shadowLightsData.dir_pos, -15.f, 250.0f);
	}
	if (ImGui::CollapsingHeader("Water Settings"))
	{
		ImGui::SliderFloat("Amplitude", &sceneData->waterData.amplitude, 0.0f, 3.0f, "%.2f", 1.0F);
		ImGui::SliderFloat("Frequency", &sceneData->waterData.frequency, 0.0f, 5.f, "%.2f", 1.0F);
		ImGui::SliderFloat("Speed", &sceneData->waterData.speed, 0.0f, 5.0f, "%.2f", 1.0F);
	}
	if (ImGui::CollapsingHeader("Bloom Effect Settings"))
	{
		ImGui::SliderFloat("Pollution Amount", &sceneData->bloomData.blurAmount, 0.f, 2.f);
		ImGui::SliderFloat("Bloom Intensity", &sceneData->bloomData.blurIntensity, 0.f, 2.f);
	}

	ImGui::End();

	// Window 2
	ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(150, 400), ImGuiCond_FirstUseEver);

	ImGui::Begin("Helper Box", nullptr, ImGuiWindowFlags_None);
	if (ImGui::Button("Toggle Wireframe")) {
		if (sceneData->tessMesh)  sceneData->tessMesh = false;
		wireframeToggle = !wireframeToggle;
	}

	ImGui::Separator();

	ImGui::Text("Mode Switch");

	if (ImGui::Button("Fly Camera Mode"))  currentMode = AppMode::FlyCam;
	if (ImGui::Button("Play Mode"))  currentMode = AppMode::Play;

	ImGui::Separator();

	ImGui::Text("Voronoi Islands");
	ImGui::SliderInt("Island Count", &sceneData->islandCount, 2, 6);

	if (ImGui::Button("Regenerate Islands")) {
		sceneData->gridSize = max(sceneData->gridSize, static_cast<int>(sceneData->islandSize * 2));
		voronoiIslands = make_unique<Voronoi::VoronoiIslands>(sceneData->gridSize, sceneData->islandCount);
		voronoiIslands->GenerateIslands();
		terrainShader->setIslands(voronoiIslands->GetIslands(), sceneData->islandSize);
		terrainShader->setBridges(voronoiIslands->GetBridges(), voronoiIslands->GetIslands());
		audioSystem.stopAllIslandAmbience();
		for (auto& island : voronoiIslands->GetIslands()) {
			float height = terrainShader->getHeight(island.position.x, island.position.z);
			XMFLOAT3 pos = XMFLOAT3(island.position.x, height, island.position.z);
			audioSystem.createIslandAmbience(pos);
		}
	}

	ImGui::Separator();

	if (ImGui::Button("Reset Entire View")) sceneData->resetView();

	ImGui::Separator();

	ImGui::Text("Teleport");

	if (ImGui::Button("Reset Camera Position"))
	{
		camera->setPosition(40.8f, 13.f, -11.8f);
		camera->setRotation(2.3f, 9.9f, 0.f);
	}
	if (ImGui::Button("Left View"))
	{
		camera->setPosition(-36.2f, 64.f, 52.6f);
		camera->setRotation(36.0f, 92.0f, 0.f);
	}
	if (ImGui::Button("Right View"))
	{
		camera->setPosition(122.8f, 74.f, 52.4f);
		camera->setRotation(48.0f, -94.3f, 0.f);
	}
	if (ImGui::Button("Opposite View"))
	{
		camera->setPosition(82.5f, 100.2f, 131.8f);
		camera->setRotation(52.0f, -168.35f, 0.f);
	}
	if (ImGui::Button("Top-down View"))
	{
		camera->setPosition(25.0f, 90.2f, 24.0f);
		camera->setRotation(66.0f, 41.0f, 0.f);
	}

	ImGui::Separator();

	ImGui::Text("Bloom - RtT Post Processing");

	if (ImGui::Button("High Bloom, High Blur"))
	{
		sceneData->bloomData.blurAmount = 2.f;
		sceneData->bloomData.blurIntensity = 2.f;
	}
	if (ImGui::Button("Low Bloom, High Blur"))
	{
		sceneData->bloomData.blurAmount = 2.f;
		sceneData->bloomData.blurIntensity = 1.f;
	}
	if (ImGui::Button("High Bloom, Low Blur"))
	{
		sceneData->bloomData.blurAmount = 0.5f;
		sceneData->bloomData.blurIntensity = 2.f;
	}
	if (ImGui::Button("Low Bloom, Low Blur"))
	{
		sceneData->bloomData.blurAmount = 0.5f;
		sceneData->bloomData.blurIntensity = 0.5f;
	}

	ImGui::Separator();

	ImGui::Text("Shadows");

	if (ImGui::Button("Toggle Spotlight"))
	{
		sceneData->toggleSpotShadow();
	}
	if (ImGui::Button("Toggle Directional Light"))
	{
		sceneData->toggleDirShadow();
	}

	ImGui::Separator();

	ImGui::Text("Tessellation");

	if (ImGui::Button("Toggle Tessellation"))
	{
		sceneData->toggleTessellation();
		wireframeToggle = !wireframeToggle;
	}

	ImGui::End();

	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

bool App1::frame()
{
	bool result;

	result = BaseApplication::frame();
	if (!result)
	{
		return false;
	}

	// Render the graphics.
	result = render();
	if (!result)
	{
		return false;
	}

	return true;
}


/*****************************    Initialize Resources    ************************************/

void App1::initComponents() {
	// Camera setup
	camera->setPosition(40.8f, 13.f, -11.8f);
	camera->setRotation(2.3f, 9.9f, 0.f);

	// Scene Data capsule setup (contains all data values)
	sceneData = new SceneData();

	// Lights setup
	spotLight = new Light();
	directionalLight = new Light();
	pointLight1 = new Light();
	pointLight2 = new Light();
	teapotSpotlights.resize(6);

	spotLight->generateOrthoMatrix((float)sceneWidth, (float)sceneHeight, 0.1f, 2000.f);
	directionalLight->generateOrthoMatrix((float)sceneWidth, (float)sceneHeight, 0.1f, 2000.f);
	pointLight1->generateOrthoMatrix((float)sceneWidth, (float)sceneHeight, 0.1f, 500);
	pointLight2->generateOrthoMatrix((float)sceneWidth, (float)sceneHeight, 0.1f, 500);

	// Audio
	if (!audioSystem.init()) MessageBox(hwnd, L"Failed to initialize audio system", L"Audio Error", MB_OK | MB_ICONERROR);

	/*****************************    Initialize individual components    ************************************/

	// Circle Dome
	circleDome = new SphereMesh(renderer->getDevice(), renderer->getDeviceContext());
	domeShader = new DomeShader(renderer->getDevice(), hwnd);
	textureMgr->loadTexture(L"dome", L"res/sky/brandon-griggs-PcAxQ_BMjnk-unsplash.jpg"); // Griggs, Brandon (2024). Unsplash. Available at: https://unsplash.com/photos/a-black-background-with-a-small-amount-of-snow-PcAxQ_BMjnk (Accessed: November 17, 2024).

	// Terrain
	topTerrain = new CubeMesh(renderer->getDevice(), renderer->getDeviceContext());
	terrainShader = new TerrainManipulation(renderer->getDevice(), hwnd);

	// Voronoi Islands
	textureMgr->loadTexture(L"island_floor", L"res/Floor_Black.jpg");
	voronoiIslands = make_unique<Voronoi::VoronoiIslands>(sceneData->gridSize, sceneData->islandCount);
	voronoiIslands->GenerateIslands();
	terrainShader->setIslands(voronoiIslands->GetIslands(), sceneData->islandSize);
	terrainShader->setBridges(voronoiIslands->GetBridges(), voronoiIslands->GetIslands());

	// Water
	water = new PlaneMesh(renderer->getDevice(), renderer->getDeviceContext());
	waterShader = new WaterShader(renderer->getDevice(), hwnd);
	textureMgr->loadTexture(L"water", L"res/blue_water.jpg"); // RoStRecords. Envato. Available at: https://elements.envato.com/pool-with-blue-water-water-surface-texture-top-vie-SXS2RKD (Accessed: November 17, 2024).

	// Moon
	moon = new SphereMesh(renderer->getDevice(), renderer->getDeviceContext());
	moonShader = new MoonShader(renderer->getDevice(), hwnd);
	textureMgr->loadTexture(L"moon", L"res/moon.jpg"); // Solar System Scope. Solar System. Available at: https://www.solarsystemscope.com/textures/ (Accessed: November 27, 2024).

	// Ghost
	ghostShader = new GhostShader(renderer->getDevice(), hwnd);
	ghost = new AModel(renderer->getDevice(), "res/Sphere.obj"); // Falconer, Ruth (2024) ‘DX Framework for CMP301’ [My Learning Space]. Abertay University. 25 September.
	textureMgr->loadTexture(L"ghost", L"res/yellow.jpg"); // Dent, Jason (2020) Unsplash. Available at: https://unsplash.com/photos/yellow-and-white-color-illustration-S53ekmu8KkE (Accessed: December 8, 2024).

	// Chromatic Aberration
	chromaticAberration = new ChromaticAberration(renderer->getDevice(), hwnd);
	simpleTexture = new SimpleTexture(renderer->getDevice(), hwnd);
	renderAberration = new RenderTexture(renderer->getDevice(), SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_NEAR, SCREEN_DEPTH);
	screenEffects = new QuadMesh(renderer->getDevice(), renderer->getDeviceContext());

	// Bloom
	renderBloom = new RenderTexture(renderer->getDevice(), SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_NEAR, SCREEN_DEPTH);
	bloomShader = new Bloom(renderer->getDevice(), hwnd);

	// Depth Shaders
	depthShader = new DepthShader(renderer->getDevice(), hwnd);
	waterDepthShader = new WaterDepthShader(renderer->getDevice(), hwnd);
	terrainDepthShader = new TerrainDepthShader(renderer->getDevice(), hwnd);

	// Shadow Maps
	shadowMap[0] = new ShadowMap(renderer->getDevice(), shadowmapWidth, shadowmapHeight);
	shadowMap[1] = new ShadowMap(renderer->getDevice(), shadowmapWidth, shadowmapHeight);

	// Teapots
	teapot = new AModel(renderer->getDevice(), "res/teapot.obj"); // Falconer, Ruth (2024) ‘DX Framework for CMP301’ [My Learning Space]. Abertay University. 03 May.
	textureMgr->loadTexture(L"teapot", L"res/snow2/snow.jpg"); // wirestock. Freepik. Available at: https://www.freepik.com/free-photo/closeup-texture-fresh-white-snow-surface_23836198.htm#fromView=search&page=1&position=1&uuid=89966487-bab0-4307-a96b-a316a9055e31 (Accessed: November 27, 2024).

	// Player, Ghost
	ghostActor = new Ghost();
	player = new Player();
	player->initialize(sceneData);

	testTess = new PlaneMesh(renderer->getDevice(), renderer->getDeviceContext());
}

/*****************************    Cleanup    ************************************/

void App1::cleanup() {
	if (circleDome) { delete circleDome; circleDome = nullptr; }
	if (domeShader) { delete domeShader; domeShader = nullptr; }
	if (renderBloom) { delete renderBloom; renderBloom = nullptr; }
	if (bloomShader) { delete bloomShader; bloomShader = nullptr; }
	if (water) { delete water; water = nullptr; }
	if (waterShader) { delete waterShader; waterShader = nullptr; }
	if (topTerrain) { delete topTerrain; topTerrain = nullptr; }
	if (terrainShader) { delete terrainShader; terrainShader = nullptr; }
	if (moon) { delete moon; moon = nullptr; }
	if (moonShader) { delete moonShader; moonShader = nullptr; }
	if (spotLight) { delete spotLight; spotLight = nullptr; }
	if (pointLight1) { delete pointLight1; pointLight1 = nullptr; }
	if (pointLight2) { delete pointLight2; pointLight2 = nullptr; }
	if (ghost) { delete ghost; ghost = nullptr; }
	if (depthShader) { delete depthShader; depthShader = nullptr; }
	if (waterDepthShader) { delete waterDepthShader; waterDepthShader = nullptr; }
	if (terrainDepthShader) { delete terrainDepthShader; terrainDepthShader = nullptr; }
	if (ghostShader) { delete ghostShader; ghostShader = nullptr; }
	if (sceneData) { delete sceneData; sceneData = nullptr; }
	if (player) { delete player; player = nullptr; }

	for (int i = 0; i < 2; ++i) {
		if (shadowMap[i]) { delete shadowMap[i]; shadowMap[i] = nullptr; }
	}

	BaseApplication::~BaseApplication();
}