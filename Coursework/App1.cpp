#include "App1.h"
#include <DirectXMath.h>
using namespace DirectX;

AppMode currentMode = AppMode::FlyCam;

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
	firefly = nullptr;
	depthShader = nullptr;
	waterDepthShader = nullptr;
	terrainDepthShader = nullptr;
	fireflyShader = nullptr;
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

	// Initialize all the resources required
	initComponents();
}

bool App1::render() {
	renderer->beginScene(0.15f, 0.1f, 0.3f, 1.0f);
	renderer->setAlphaBlending(true);

	// Set up the lights from Scene Data
	XMFLOAT3 playerPos = player->getPosition();

	// Update SceneData with player's light position
	sceneData->setPointLight1Position(playerPos.x, playerPos.y + 2.f, playerPos.z);

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


	XMMATRIX worldMatrix = renderer->getWorldMatrix();
	XMMATRIX viewMatrix = camera->getViewMatrix();
	XMMATRIX projectionMatrix = renderer->getProjectionMatrix();
	XMMATRIX identity = XMMatrixIdentity();

	getShadowDepthMap(worldMatrix, viewMatrix, projectionMatrix, identity);
	finalRender(worldMatrix, viewMatrix, projectionMatrix, identity);


	float dt = timer->getTime();
	if (dt < 1e-6f) dt = 0.016f;

	if (currentMode == AppMode::FlyCam) camera->update();
	else if (currentMode == AppMode::Play) player->update(dt, input, terrainShader);

	// --- compute velocity ---
	XMFLOAT3 curPos = camera->getPosition();
	XMFLOAT3 vel;
	vel.x = (curPos.x - m_lastCamPos.x) / dt;
	vel.y = (curPos.y - m_lastCamPos.y) / dt;
	vel.z = (curPos.z - m_lastCamPos.z) / dt;
	m_camVelocity = XMLoadFloat3(&vel);

	// --- collision with terrain (sliding) ---
	float terrainY = terrainShader->getHeight(curPos.x, curPos.z);
	float minY = terrainY + m_camEyeHeight;

	if (curPos.y < minY) {
		// clamp above surface
		curPos.y = minY;

		// compute sliding: remove into-terrain component
		XMFLOAT3 normal = terrainShader->getNormal(curPos.x, curPos.z);
		XMVECTOR N = XMLoadFloat3(&normal);
		XMVECTOR proj = XMVectorScale(N, XMVectorGetX(XMVector3Dot(m_camVelocity, N)));
		XMVECTOR slideVel = XMVectorSubtract(m_camVelocity, proj);

		// reset position based on slide
		XMFLOAT3 sv;
		XMStoreFloat3(&sv, slideVel);
		curPos.x = m_lastCamPos.x + sv.x * dt;
		curPos.z = m_lastCamPos.z + sv.z * dt;

		camera->setPosition(curPos.x, curPos.y, curPos.z);
		m_camVelocity = slideVel;
	}

	m_lastCamPos = curPos;

	if (currentMode == AppMode::Play)
	{
		player->update(dt, input, terrainShader);
		player->handleMouseLook(input, dt, this->wnd, this->sceneWidth, this->sceneHeight);

		// Set camera position and rotation based on player
		XMFLOAT3 camPos = player->getCameraPosition();

		camera->setPosition(camPos.x, camPos.y, camPos.z);
		camera->setRotation(player->getRotation().x, player->getRotation().y, 0.0f);
		camera->update();
	}



	gui();

	renderer->endScene();

	return true;
}

/*****************************    Renders    ************************************/

// Shadow Depth Map
// Two types of lights are used for shadow depth mapping: a directional light and a spotlight. The shadow map is set up for rendering by binding the depth buffer and disabling colour rendering. Shadows are created by rendering the scene from the perspective of each light, capturing depth information. This data is then used in the final render to produce shadows. The position is calculated and updated in the lookAt function, which is used to generate the orthographic matrix. After generating the view matrix for both lights and rendering the meshes with their respective shaders, the render target is reset to the back buffer, and the viewport is restored.

void App1::getShadowDepthMap(const XMMATRIX& worldMatrix, const XMMATRIX& viewMatrix, const XMMATRIX& projectionMatrix, const XMMATRIX& identityMatrix) {

	Light* lights[2] = { spotLight, directionalLight };

	// Repeat for each light source
	for (int i = 0; i < 2; ++i) {

		Light* currentLight = lights[i];

		// 1. Set Shadow Map as Render Target
		shadowMap[i]->BindDsvAndSetNullRenderTarget(renderer->getDeviceContext());

		// Set the light direction and position
		XMFLOAT3 lightPos = currentLight->getPosition();
		currentLight->setLookAt(lightPos.x, lightPos.y, lightPos.z);

		// 2. Get matrix based on light's position and direction
		currentLight->generateViewMatrix();
		XMMATRIX lightViewMatrix = currentLight->getViewMatrix();
		XMMATRIX lightProjectionMatrix = currentLight->getOrthoMatrix();

		// 3. Apply Depth Shader to the meshes

		// Firefly
		XMMATRIX fireflyWorldMatrix = XMMatrixTranslation(sceneData->fireflyData.objPos[0], sceneData->fireflyData.objPos[1], sceneData->fireflyData.objPos[2]) * worldMatrix;

		firefly->sendData(renderer->getDeviceContext());
		depthShader->setShaderParameters(renderer->getDeviceContext(), fireflyWorldMatrix, lightViewMatrix, lightProjectionMatrix);
		depthShader->render(renderer->getDeviceContext(), firefly->getIndexCount());

		// Terrain - own vertex shader needed to calculate displacements to cast shows correclty on it
		topTerrain->sendData(renderer->getDeviceContext(), D3D_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST);
		terrainDepthShader->setShaderParameters(renderer->getDeviceContext(), worldMatrix, viewMatrix, projectionMatrix, textureMgr->getTexture(L"terrain_heightmap"), textureMgr->getTexture(L"colour_1_height"), camera);
		terrainDepthShader->render(renderer->getDeviceContext(), topTerrain->getIndexCount());

		// Water - own vertex shader needed to calculate displacements to cast shows correclty on it
		XMMATRIX waterWorldMatrix = XMMatrixTranslation(1.f, 0.0f, 1.0f) * worldMatrix;
		water->sendData(renderer->getDeviceContext());
		waterDepthShader->setShaderParameters(renderer->getDeviceContext(), waterWorldMatrix, viewMatrix, projectionMatrix, sceneData);
		waterDepthShader->render(renderer->getDeviceContext(), water->getIndexCount());
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

		renderDome(worldMatrix, viewMatrix, projectionMatrix);
		renderTerrain(worldMatrix, viewMatrix, projectionMatrix);
		renderFirefly(worldMatrix, viewMatrix, projectionMatrix);
		//renderWater(worldMatrix, viewMatrix, projectionMatrix);
		renderMoon(worldMatrix, viewMatrix, projectionMatrix);
	}
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

	XMMATRIX skyDomeWorldMatrix = XMMatrixScaling(700.f, 200.f, 700.f) * XMMatrixTranslation(50.f, 30.f, 50.f) * worldMatrix;

	circleDome->sendData(renderer->getDeviceContext());
	domeShader->setShaderParameters(renderer->getDeviceContext(), skyDomeWorldMatrix, viewMatrix, projectionMatrix, renderBloom->getShaderResourceView(), sceneData);
	domeShader->render(renderer->getDeviceContext(), circleDome->getIndexCount());
}

// Procedural Generation of Voronoi Islands
// Based on the number of islands, that many Voronoi regions are created in voronoiIslands.GenerateVoronoiRegions(). Then, inside each Voronoi region, an island is spawn with a random position and rotation. After that, each island connects to one other island with a bridge. The islands have collision detection with the Player (Play Mode) or the Camera (Fly Mode).

void App1::generateIslands(const XMMATRIX& worldMatrix, const XMMATRIX& viewMatrix, const XMMATRIX& projectionMatrix) {

	const float floorSize = 50.0f;

	for (const auto& island : voronoiIslands->GetIslands()) {
		if (!island.initialized) continue;

		XMMATRIX islandWorld = XMMatrixScaling(floorSize, 1.0f, floorSize) *
			XMMatrixRotationY(island.rotationY) *
			XMMatrixTranslation(island.position.x, island.position.y, island.position.z);

		topTerrain->sendData(renderer->getDeviceContext(), D3D_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST);
		terrainShader->setShaderParameters(renderer->getDeviceContext(), islandWorld, viewMatrix, projectionMatrix,
			textureMgr->getTexture(L"terrain_heightmapCUT"),
			textureMgr->getTexture(L"colour_1_heightCUT"),
			textureMgr->getTexture(L"colour_1"),
			textureMgr->getTexture(L"colour_2"),
			sceneData->shadowLightsData.enableSpotShadow ? shadowMap[0]->getDepthMapSRV() : nullptr,
			sceneData->shadowLightsData.enableDirShadow ? shadowMap[1]->getDepthMapSRV() : nullptr,
			camera, spotLight, directionalLight, sceneData);
		terrainShader->render(renderer->getDeviceContext(), topTerrain->getIndexCount());
	}
}

void App1::generateBridges(const XMMATRIX& worldMatrix, const XMMATRIX& viewMatrix, const XMMATRIX& projectionMatrix) {
	const float bridgeWidth = 2.0f;
	const float bridgeHeight = 0.5f;
	const float halfIslandSize = 25.0f;

	for (const auto& bridge : voronoiIslands->GetBridges()) {
		const auto& a = voronoiIslands->GetIslands()[bridge.islandA];
		const auto& b = voronoiIslands->GetIslands()[bridge.islandB];

		XMVECTOR posA = XMLoadFloat3(&a.position);
		XMVECTOR posB = XMLoadFloat3(&b.position);
		XMVECTOR dir = XMVector3Normalize(posB - posA);
		float exitAngle = atan2(XMVectorGetZ(dir), XMVectorGetX(dir));
		XMFLOAT3 exitPointA = a.position;
		XMFLOAT3 entryPointB = b.position;

		if (fabs(XMVectorGetX(dir)) > fabs(XMVectorGetZ(dir))) {
			exitPointA.x += (XMVectorGetX(dir) > 0 ? halfIslandSize : -halfIslandSize);
			exitPointA.z += tan(exitAngle) * halfIslandSize;
			entryPointB.x += (XMVectorGetX(dir) > 0 ? -halfIslandSize : halfIslandSize);
			entryPointB.z += tan(exitAngle) * -halfIslandSize;
		}
		else {
			exitPointA.z += (XMVectorGetZ(dir) > 0 ? halfIslandSize : -halfIslandSize);
			exitPointA.x += 1.0f / tan(exitAngle) * halfIslandSize;
			entryPointB.z += (XMVectorGetZ(dir) > 0 ? -halfIslandSize : halfIslandSize);
			entryPointB.x += 1.0f / tan(exitAngle) * -halfIslandSize;
		}

		XMVECTOR bridgeStart = XMLoadFloat3(&exitPointA);
		XMVECTOR bridgeEnd = XMLoadFloat3(&entryPointB);
		XMVECTOR bridgeCenter = (bridgeStart + bridgeEnd) * 0.5f;
		float bridgeLength = XMVectorGetX(XMVector3Length(bridgeEnd - bridgeStart));

		XMMATRIX bridgeWorld = XMMatrixScaling(bridgeLength, bridgeHeight, bridgeWidth) *
			XMMatrixRotationY(-atan2(XMVectorGetZ(dir), XMVectorGetX(dir))) *
			XMMatrixTranslation(
				XMVectorGetX(bridgeCenter),
				bridgeHeight * 0.5f,
				XMVectorGetZ(bridgeCenter)
			);

		topTerrain->sendData(renderer->getDeviceContext(), D3D_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST);
		terrainShader->setShaderParameters(renderer->getDeviceContext(), bridgeWorld, viewMatrix, projectionMatrix,
			textureMgr->getTexture(L"bridge_texture"),
			textureMgr->getTexture(L"colour_1_heightCUT"),
			textureMgr->getTexture(L"colour_1"),
			textureMgr->getTexture(L"colour_2"),
			sceneData->shadowLightsData.enableSpotShadow ? shadowMap[0]->getDepthMapSRV() : nullptr,
			sceneData->shadowLightsData.enableDirShadow ? shadowMap[1]->getDepthMapSRV() : nullptr,
			camera, spotLight, directionalLight, sceneData);
		terrainShader->render(renderer->getDeviceContext(), topTerrain->getIndexCount());
	}
}

void App1::renderTerrain(const XMMATRIX& worldMatrix, const XMMATRIX& viewMatrix, const XMMATRIX& projectionMatrix) {
	renderer->setCullOn(false);
	generateIslands(worldMatrix, viewMatrix, projectionMatrix);
	generateBridges(worldMatrix, viewMatrix, projectionMatrix);
	renderer->setCullOn(true);
}


void App1::renderFirefly(const XMMATRIX& worldMatrix, const XMMATRIX& viewMatrix, const XMMATRIX& projectionMatrix) {
	XMMATRIX fireflyWorldMatrix = XMMatrixTranslation(sceneData->fireflyData.objPos[0], sceneData->fireflyData.objPos[1], sceneData->fireflyData.objPos[2]) * worldMatrix;

	firefly->sendData(renderer->getDeviceContext());
	fireflyShader->setShaderParameters(renderer->getDeviceContext(), fireflyWorldMatrix, viewMatrix, projectionMatrix, textureMgr->getTexture(L"firefly"), camera, spotLight, directionalLight, sceneData);
	fireflyShader->render(renderer->getDeviceContext(), firefly->getIndexCount());
}

void App1::renderWater(const XMMATRIX& worldMatrix, const XMMATRIX& viewMatrix, const XMMATRIX& projectionMatrix) {
	XMMATRIX waterWorldMatrix = XMMatrixTranslation(1.f, 0.0f, 1.0f) * worldMatrix;

	renderer->setCullBack(true);

	// GUI conditional statement - Toggle shadows
	water->sendData(renderer->getDeviceContext());
	waterShader->setShaderParameters(renderer->getDeviceContext(), waterWorldMatrix, viewMatrix, projectionMatrix, textureMgr->getTexture(L"water"), sceneData->shadowLightsData.enableSpotShadow ? shadowMap[0]->getDepthMapSRV() : nullptr, sceneData->shadowLightsData.enableDirShadow ? shadowMap[1]->getDepthMapSRV() : nullptr, camera, spotLight, directionalLight, sceneData);
	waterShader->render(renderer->getDeviceContext(), water->getIndexCount());

	renderer->setCullBack(false);
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
	if (ImGui::CollapsingHeader("Model Settings"))
	{
		ImGui::Text("Firefly");

		ImGui::SliderFloat3("Firefly Position", sceneData->fireflyData.objPos, -15.f, 210.0f);
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
	ImGui::SliderInt("Grid Size", &gridSize, islandSize, 500);
	ImGui::SliderInt("Island Count", &islandCount, 1, 4);
	ImGui::SliderFloat("Min Distance", &minIslandDistance, islandSize, 150.0f);

	if (ImGui::Button("Regenerate Islands")) {
		gridSize = max(gridSize, static_cast<int>(islandSize * 2));
		voronoiIslands = make_unique<VoronoiIslands>(gridSize, islandCount);
		voronoiIslands->GenerateIslands();
	}

	ImGui::Text("Island Count: %d", voronoiIslands->GetIslands().size());
	for (int i = 0; i < voronoiIslands->GetIslands().size(); ++i) {
		const auto& island = voronoiIslands->GetIslands()[i];
		float rotationDegrees = XMConvertToDegrees(island.rotationY);

		ImGui::Text("Island %d:", i + 1);
		ImGui::BulletText("Position: (%.1f, %.1f)", island.position.x, island.position.z);
		ImGui::BulletText("Rotation: %.2f rad (%.1f°)",
			island.rotationY,
			rotationDegrees);

		// Optional: Add a separator between islands for better readability
		if (i < voronoiIslands->GetIslands().size() - 1) {
			ImGui::Separator();
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

	ImGui::Text("Firefly Position");

	if (ImGui::Button("Point Light"))
	{
		sceneData->fireflyData.objPos[0] = 68.955f;
		sceneData->fireflyData.objPos[1] = 11.866f;
		sceneData->fireflyData.objPos[2] = 84.265f;
	}

	if (ImGui::Button("Shadow Position"))
	{
		sceneData->fireflyData.objPos[0] = 58.881f;
		sceneData->fireflyData.objPos[1] = 8.507f;
		sceneData->fireflyData.objPos[2] = 68.2f;
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

	spotLight->generateOrthoMatrix((float)sceneWidth, (float)sceneHeight, 0.1f, 2000.f);
	directionalLight->generateOrthoMatrix((float)sceneWidth, (float)sceneHeight, 0.1f, 2000.f);
	pointLight1->generateOrthoMatrix((float)sceneWidth, (float)sceneHeight, 0.1f, 500);
	pointLight2->generateOrthoMatrix((float)sceneWidth, (float)sceneHeight, 0.1f, 500);

	/*****************************    Initialize individual components    ************************************/

	// Circle Dome
	circleDome = new SphereMesh(renderer->getDevice(), renderer->getDeviceContext());
	domeShader = new DomeShader(renderer->getDevice(), hwnd);
	textureMgr->loadTexture(L"dome", L"res/sky/brandon-griggs-PcAxQ_BMjnk-unsplash.jpg"); // Griggs, Brandon (2024). Unsplash. Available at: https://unsplash.com/photos/a-black-background-with-a-small-amount-of-snow-PcAxQ_BMjnk (Accessed: November 17, 2024).

	// Terrain
	topTerrain = new CubeMesh(renderer->getDevice(), renderer->getDeviceContext());
	terrainShader = new TerrainManipulation(renderer->getDevice(), hwnd);
	textureMgr->loadTexture(L"terrain_heightmap", L"res/HeightMaps/11_2.jpg"); // © Mapzen, OpenStreetMap, and others. Unreal PNG Heightmap. Available at: https://manticorp.github.io/unrealheightmap/index.html (Accessed: November 12, 2024).
	textureMgr->loadTexture(L"colour_1_height", L"res/snowdust/textures/snow_field_aerial_height_2k.png"); // Tuytel, Rob (2021). Poly Haven. Available at: https://polyhaven.com/a/snow_field_aerial (Accessed: November 27, 2024).
	textureMgr->loadTexture(L"colour_1", L"res/snowdust/textures/snow_field_aerial_col_2k.jpg"); // Tuytel, Rob (2021). Poly Haven. Available at: https://polyhaven.com/a/snow_field_aerial (Accessed: November 27, 2024).
	textureMgr->loadTexture(L"colour_2", L"res/snow2/snow.jpg"); // wirestock. Freepik. Available at: https://www.freepik.com/free-photo/closeup-texture-fresh-white-snow-surface_23836198.htm#fromView=search&page=1&position=1&uuid=89966487-bab0-4307-a96b-a316a9055e31 (Accessed: November 27, 2024).

	// Voronoi Islands
	voronoiIslands = make_unique<VoronoiIslands>(gridSize, islandCount);
	voronoiIslands->GenerateIslands();

	// Water
	water = new PlaneMesh(renderer->getDevice(), renderer->getDeviceContext());
	waterShader = new WaterShader(renderer->getDevice(), hwnd);
	textureMgr->loadTexture(L"water", L"res/blue_water.jpg"); // RoStRecords. Envato. Available at: https://elements.envato.com/pool-with-blue-water-water-surface-texture-top-vie-SXS2RKD (Accessed: November 17, 2024).

	// Moon
	moon = new SphereMesh(renderer->getDevice(), renderer->getDeviceContext());
	moonShader = new MoonShader(renderer->getDevice(), hwnd);
	textureMgr->loadTexture(L"moon", L"res/moon.jpg"); // Solar System Scope. Solar System. Available at: https://www.solarsystemscope.com/textures/ (Accessed: November 27, 2024).

	// Firefly
	fireflyShader = new FireflyShader(renderer->getDevice(), hwnd);
	firefly = new AModel(renderer->getDevice(), "res/Sphere.obj"); // Falconer, Ruth (2024) ‘DX Framework for CMP301’ [My Learning Space]. Abertay University. 25 September.
	textureMgr->loadTexture(L"firefly", L"res/yellow.jpg"); // Dent, Jason (2020) Unsplash. Available at: https://unsplash.com/photos/yellow-and-white-color-illustration-S53ekmu8KkE (Accessed: December 8, 2024).

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

	// Player
	player = new Player();
	player->setPosition(17.f, 13.f, -9.f);

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
	if (firefly) { delete firefly; firefly = nullptr; }
	if (depthShader) { delete depthShader; depthShader = nullptr; }
	if (waterDepthShader) { delete waterDepthShader; waterDepthShader = nullptr; }
	if (terrainDepthShader) { delete terrainDepthShader; terrainDepthShader = nullptr; }
	if (fireflyShader) { delete fireflyShader; fireflyShader = nullptr; }
	if (sceneData) { delete sceneData; sceneData = nullptr; }
	if (player) { delete player; player = nullptr; }

	for (int i = 0; i < 2; ++i) {
		if (shadowMap[i]) { delete shadowMap[i]; shadowMap[i] = nullptr; }
	}

	BaseApplication::~BaseApplication();
}