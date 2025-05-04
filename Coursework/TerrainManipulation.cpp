#include "TerrainManipulation.h"
#include <cmath>

static float distPointLineSegment2D(
	float px, float pz,
	float x0, float z0,
	float x1, float z1)
{
	float vx = x1 - x0, vz = z1 - z0;
	float wx = px - x0, wz = pz - z0;
	float c1 = vx * wx + vz * wz;
	float c2 = vx * vx + vz * vz;
	float t = (c2 < 1e-6f) ? 0.f : c1 / c2;
	t = max(0.f, min(1.f, t));
	float ix = x0 + t * vx, iz = z0 + t * vz;
	float dx = px - ix, dz = pz - iz;
	return sqrtf(dx * dx + dz * dz);
}


TerrainManipulation::TerrainManipulation(ID3D11Device* device, HWND hwnd) : BaseShader(device, hwnd)
{
	initShader(L"terrain_vs.cso", L"terrain_hs.cso", L"terrain_ds.cso", L"terrain_ps.cso");
}

TerrainManipulation::~TerrainManipulation()
{
	// Release the matrix constant buffer.
	if (matrixBuffer)
	{
		matrixBuffer->Release();
		matrixBuffer = 0;
	}

	// Release the layout.
	if (layout)
	{
		layout->Release();
		layout = 0;
	}

	if (terrainSampleState)
	{
		terrainSampleState->Release();
		terrainSampleState = 0;
	}

	if (textureSamplerState)
	{
		textureSamplerState->Release();
		textureSamplerState = 0;
	}

	if (shadowSample1)
	{
		shadowSample1->Release();
		shadowSample1 = 0;
	}

	if (shadowSample2)
	{
		shadowSample2->Release();
		shadowSample2 = 0;
	}

	// Release the light constant buffer.
	if (lightBuffer)
	{
		lightBuffer->Release();
		lightBuffer = 0;
	}

	// Release the camera constant buffer.
	if (cameraBuffer)
	{
		cameraBuffer->Release();
		cameraBuffer = 0;
	}
	if (sonarBuffer)
	{
		sonarBuffer->Release();
		sonarBuffer = 0;
	}

	//Release base shader components
	BaseShader::~BaseShader();
}

void TerrainManipulation::initShader(const wchar_t* vsFilename, const wchar_t* psFilename)
{
	// Load (+ compile) shader files
	loadVertexShader(vsFilename);
	loadPixelShader(psFilename);
}

void TerrainManipulation::initShader(const wchar_t* vsFilename, const wchar_t* hsFilename, const wchar_t* dsFilename, const wchar_t* psFilename)
{
	D3D11_BUFFER_DESC bufferDesc;
	D3D11_BUFFER_DESC sonarBufferDesc;
	D3D11_SAMPLER_DESC samplerDesc;
	D3D11_SAMPLER_DESC spotShadowDesc;

	// InitShader must be overwritten and it will load both vertex and pixel shaders + setup buffers
	initShader(vsFilename, psFilename);

	// Load other required shaders.
	loadHullShader(hsFilename);
	loadDomainShader(dsFilename);

	// Setup the description of the dynamic matrix constant buffer that is in the vertex shader.
	bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	bufferDesc.ByteWidth = sizeof(MatrixBufferType);
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	bufferDesc.MiscFlags = 0;
	bufferDesc.StructureByteStride = 0;
	renderer->CreateBuffer(&bufferDesc, NULL, &matrixBuffer);

	// Setup light buffer
	// Setup the description of the light dynamic constant buffer that is in the pixel shader.
	// Note that ByteWidth always needs to be a multiple of 16 if using D3D11_BIND_CONSTANT_BUFFER or CreateBuffer will fail.
	bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	bufferDesc.ByteWidth = sizeof(LightBufferType);
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	bufferDesc.MiscFlags = 0;
	bufferDesc.StructureByteStride = 0;
	renderer->CreateBuffer(&bufferDesc, NULL, &lightBuffer);

	// Setup the description of the camera dynamic constant buffer that is in the vertex shader.
	bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	bufferDesc.ByteWidth = sizeof(CameraBufferType);
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	bufferDesc.MiscFlags = 0;
	bufferDesc.StructureByteStride = 0;
	renderer->CreateBuffer(&bufferDesc, NULL, &cameraBuffer);

	sonarBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	sonarBufferDesc.ByteWidth = sizeof(SonarBufferType);
	sonarBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	sonarBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	sonarBufferDesc.MiscFlags = 0;
	sonarBufferDesc.StructureByteStride = 0;
	renderer->CreateBuffer(&sonarBufferDesc, NULL, &sonarBuffer);

	// Create a terrain sampler state description.
	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.MipLODBias = 0.0f;
	samplerDesc.MaxAnisotropy = 1;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	samplerDesc.BorderColor[0] = 0;
	samplerDesc.BorderColor[1] = 0;
	samplerDesc.BorderColor[2] = 0;
	samplerDesc.BorderColor[3] = 0;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
	renderer->CreateSamplerState(&samplerDesc, &terrainSampleState);

	// Create texture sampler state description
	samplerDesc.Filter = D3D11_FILTER_ANISOTROPIC;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.MipLODBias = -0.5f;
	samplerDesc.MaxAnisotropy = 8;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
	renderer->CreateSamplerState(&samplerDesc, &textureSamplerState);

	// Sampler for shadow map sampling.
	spotShadowDesc.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT; // PCF for smooth edges
	spotShadowDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	spotShadowDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	spotShadowDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	spotShadowDesc.ComparisonFunc = D3D11_COMPARISON_LESS_EQUAL; // Compare depths for shadows
	spotShadowDesc.BorderColor[0] = 1.0f;
	spotShadowDesc.BorderColor[1] = 1.0f;
	spotShadowDesc.BorderColor[2] = 1.0f;
	spotShadowDesc.BorderColor[3] = 1.0f;
	spotShadowDesc.MinLOD = 0;
	spotShadowDesc.MaxLOD = D3D11_FLOAT32_MAX; // Use all mip levels
	renderer->CreateSamplerState(&spotShadowDesc, &shadowSample1);
	renderer->CreateSamplerState(&spotShadowDesc, &shadowSample2);
}

bool TerrainManipulation::isOnTerrain(float x, float z) const
{
	if (!m_islands) return false;

	constexpr float fullMesh = 50.0f;
	float allowed = (m_regionSize * 0.5) + fullMesh;

	for (const auto& isl : *m_islands)
	{
		// Step 1: Translate the point into island local space
		float localX = x - isl.position.x;
		float localZ = z - isl.position.z;

		// Step 2: Apply inverse rotation
		float cosR = cosf(isl.rotationY);  // Inverse rotation
		float sinR = sinf(isl.rotationY);

		float rotatedX = localX * cosR - localZ * sinR;
		float rotatedZ = localX * sinR + localZ * cosR;

		// Step 3: Check if inside the axis-aligned 50x50 box
		if (fabsf(rotatedX) <= fullMesh && fabsf(rotatedZ) <= fullMesh)
			return true;
	}
	return false;
}

bool TerrainManipulation::onBridge(float x, float z) const {
	for (auto& b : m_bridges) {
		float d = distPointLineSegment2D(
			x, z,
			b.first.x, b.first.z,
			b.second.x, b.second.z
		);
		if (d < m_bridgeWidth * 0.5f)
			return true;
	}
	return false;
}

float TerrainManipulation::getHeight(float x, float z) const
{
	return sinf(x * HEIGHT_FREQ) * cosf(z * HEIGHT_FREQ) * HEIGHT_AMPLITUDE;
}

XMFLOAT3 TerrainManipulation::getNormal(float x, float z) const
{
	float hL = getHeight(x - NORMAL_DELTA, z);
	float hR = getHeight(x + NORMAL_DELTA, z);
	float hD = getHeight(x, z - NORMAL_DELTA);
	float hU = getHeight(x, z + NORMAL_DELTA);

	XMFLOAT3 n{ hL - hR, 2 * NORMAL_DELTA, hD - hU };
	XMVECTOR norm = XMVector3Normalize(XMLoadFloat3(&n));
	XMStoreFloat3(&n, norm);
	return n;
}


void TerrainManipulation::setShaderParameters(ID3D11DeviceContext* deviceContext, const XMMATRIX& worldMatrix, const XMMATRIX& viewMatrix, const XMMATRIX& projectionMatrix, bool sonarActive, XMFLOAT3 sonarOrigin, float sonarRadius, ID3D11ShaderResourceView* terrain, ID3D11ShaderResourceView* texture_height, ID3D11ShaderResourceView* texture_colour, ID3D11ShaderResourceView* texture_colour1, ID3D11ShaderResourceView* depth1, ID3D11ShaderResourceView* depth2, Camera* camera, Light* light, Light* directionalLight, SceneData* sceneData)
{
	HRESULT result;
	D3D11_MAPPED_SUBRESOURCE mappedResource;
	XMMATRIX tworld, tview, tproj, tLightViewMatrix, tLightProjectionMatrix, tLightViewMatrix2, tLightProjectionMatrix2;

	// Transpose the matrices to prepare them for the shader.
	tworld = XMMatrixTranspose(worldMatrix);
	tview = XMMatrixTranspose(viewMatrix);
	tproj = XMMatrixTranspose(projectionMatrix);
	tLightViewMatrix = XMMatrixTranspose(light->getViewMatrix());
	tLightProjectionMatrix = XMMatrixTranspose(light->getOrthoMatrix());
	tLightViewMatrix2 = XMMatrixTranspose(directionalLight->getViewMatrix());
	tLightProjectionMatrix2 = XMMatrixTranspose(directionalLight->getOrthoMatrix());

	MatrixBufferType* dataPtr;
	result = deviceContext->Map(matrixBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	dataPtr = (MatrixBufferType*)mappedResource.pData;
	dataPtr->world = tworld;
	dataPtr->view = tview;
	dataPtr->projection = tproj;
	dataPtr->lightView = tLightViewMatrix;
	dataPtr->lightProjection = tLightProjectionMatrix;
	dataPtr->lightView2 = tLightViewMatrix2;
	dataPtr->lightProjection2 = tLightProjectionMatrix2;
	deviceContext->Unmap(matrixBuffer, 0);
	deviceContext->DSSetConstantBuffers(0, 1, &matrixBuffer);

	CameraBufferType* cameraPtr;
	deviceContext->Map(cameraBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	cameraPtr = (CameraBufferType*)mappedResource.pData;
	cameraPtr->cameraPosition = camera->getPosition();
	cameraPtr->padding = 0.0f;
	deviceContext->Unmap(cameraBuffer, 0);
	deviceContext->HSSetConstantBuffers(0, 1, &cameraBuffer);
	deviceContext->DSSetConstantBuffers(1, 1, &cameraBuffer);

	//Additional
	// Send light data to pixel shader
	LightBufferType* lightPtr;
	deviceContext->Map(lightBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	lightPtr = (LightBufferType*)mappedResource.pData;

	lightPtr->ambientColour = XMFLOAT4(sceneData->lightData.ambientColour);
	lightPtr->diffuseColour = XMFLOAT4(sceneData->lightData.diffuseColour);

	lightPtr->pointLight1Position = XMFLOAT3(sceneData->lightData.pointLight_pos1);
	lightPtr->pointLight1Radius = sceneData->lightData.pointLightRadius[0];
	lightPtr->pointLight1Colour = XMFLOAT4(sceneData->lightData.pointLight1Colour);

	lightPtr->pointLight2Position = XMFLOAT3(sceneData->lightData.pointLight_pos2);
	lightPtr->pointLight2Radius = sceneData->lightData.pointLightRadius[1];
	lightPtr->pointLight2Colour = XMFLOAT4(sceneData->lightData.pointLight2Colour);

	lightPtr->spotlightColour = XMFLOAT4(sceneData->shadowLightsData.spotColour);
	lightPtr->spotlightCutoff = sceneData->shadowLightsData.spotCutoff;
	lightPtr->spotlightDirection = XMFLOAT3(sceneData->shadowLightsData.lightDirections[0]);
	lightPtr->spotlightFalloff = sceneData->shadowLightsData.spotFalloff;

	lightPtr->moonPos = XMFLOAT3(sceneData->moonData.moon_pos);
	lightPtr->specularColour = XMFLOAT4(sceneData->lightData.specularColour);
	lightPtr->specularPower = sceneData->lightData.spec_pow;

	lightPtr->directionalLightDirection = XMFLOAT3(sceneData->shadowLightsData.lightDirections[1]);
	lightPtr->directionalColour = XMFLOAT4(sceneData->shadowLightsData.dirColour);

	deviceContext->Unmap(lightBuffer, 0);
	deviceContext->PSSetConstantBuffers(0, 1, &lightBuffer);

	SonarBufferType* sonarPtr;
	deviceContext->Map(sonarBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	sonarPtr = (SonarBufferType*)mappedResource.pData;
	sonarPtr->sonarOrigin = sonarOrigin;
	sonarPtr->sonarRadius = sonarRadius;
	sonarPtr->sonarActive = sonarActive;
	sonarPtr->padding = XMFLOAT3(0.f, 0.f, 0.f); // Padding to ensure the structure is 16-byte aligned
	deviceContext->Unmap(sonarBuffer, 0);
	deviceContext->PSSetConstantBuffers(1, 1, &sonarBuffer);

	// Pixel shader
	deviceContext->PSSetShaderResources(0, 1, &terrain); // Main height map
	deviceContext->PSSetSamplers(0, 1, &terrainSampleState);

	deviceContext->PSSetShaderResources(1, 1, &texture_height);
	deviceContext->PSSetSamplers(1, 1, &textureSamplerState);

	deviceContext->PSSetShaderResources(2, 1, &texture_colour);
	deviceContext->PSSetSamplers(2, 1, &textureSamplerState);

	deviceContext->PSSetShaderResources(3, 1, &texture_colour1);
	deviceContext->PSSetSamplers(3, 1, &textureSamplerState);

	deviceContext->PSSetShaderResources(4, 1, &depth1);
	deviceContext->PSSetSamplers(4, 1, &shadowSample1);

	deviceContext->PSSetShaderResources(5, 1, &depth2);
	deviceContext->PSSetSamplers(5, 1, &shadowSample2);

	// Domain Shader

	deviceContext->DSSetShaderResources(0, 1, &terrain);
	deviceContext->DSSetSamplers(0, 1, &terrainSampleState);

	deviceContext->DSSetShaderResources(1, 1, &texture_height);
	deviceContext->DSSetSamplers(1, 1, &textureSamplerState);
}