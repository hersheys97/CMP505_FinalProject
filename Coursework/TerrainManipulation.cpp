#include "TerrainManipulation.h"

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

	// Create a terrain sampler state description.
	samplerDesc.Filter = D3D11_FILTER_ANISOTROPIC;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.MipLODBias = 0.0f;
	samplerDesc.MaxAnisotropy = 1;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
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

void TerrainManipulation::setShaderParameters(ID3D11DeviceContext* deviceContext, const XMMATRIX& worldMatrix, const XMMATRIX& viewMatrix, const XMMATRIX& projectionMatrix, ID3D11ShaderResourceView* terrain, ID3D11ShaderResourceView* texture_height, ID3D11ShaderResourceView* texture_colour, ID3D11ShaderResourceView* texture_colour1, ID3D11ShaderResourceView* depth1, ID3D11ShaderResourceView* depth2, Camera* camera, Light* light, Light* directionalLight, SceneData* sceneData)
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