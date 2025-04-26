#include "WaterShader.h"

WaterShader::WaterShader(ID3D11Device* device, HWND hwnd) : BaseShader(device, hwnd)
{
	initShader(L"water_vs.cso", L"water_ps.cso");
}

WaterShader::~WaterShader()
{
	// Release the matrix constant buffer.
	if (matrixBuffer)
	{
		matrixBuffer->Release();
		matrixBuffer = 0;
	}

	if (layout)
	{
		layout->Release();
		layout = 0;
	}

	if (waterSampleState)
	{
		waterSampleState->Release();
		waterSampleState = 0;
	}

	if (sampleStateShadow)
	{
		sampleStateShadow->Release();
		sampleStateShadow = 0;
	}

	if (timeBuffer)
	{
		timeBuffer->Release();
		timeBuffer = 0;
	}

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

void WaterShader::initShader(const wchar_t* vsFilename, const wchar_t* psFilename)
{
	D3D11_BUFFER_DESC bufferDesc;
	D3D11_SAMPLER_DESC waterSamplerDesc;
	D3D11_SAMPLER_DESC shadowSamplerDesc;

	// Load (+ compile) shader files
	loadVertexShader(vsFilename);
	loadPixelShader(psFilename);

	// Setup the description of the dynamic matrix constant buffer that is in the vertex shader.
	bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	bufferDesc.ByteWidth = sizeof(MatrixBufferType);
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	bufferDesc.MiscFlags = 0;
	bufferDesc.StructureByteStride = 0;
	renderer->CreateBuffer(&bufferDesc, NULL, &matrixBuffer);

	// Setup the description of the camera dynamic constant buffer that is in the vertex shader.
	bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	bufferDesc.ByteWidth = sizeof(CameraBufferType);
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	bufferDesc.MiscFlags = 0;
	bufferDesc.StructureByteStride = 0;
	renderer->CreateBuffer(&bufferDesc, NULL, &cameraBuffer);

	bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	bufferDesc.ByteWidth = sizeof(TimeBufferType);
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	bufferDesc.MiscFlags = 0;
	bufferDesc.StructureByteStride = 0;
	renderer->CreateBuffer(&bufferDesc, NULL, &timeBuffer);

	bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	bufferDesc.ByteWidth = sizeof(LightBufferType);
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	bufferDesc.MiscFlags = 0;
	bufferDesc.StructureByteStride = 0;
	renderer->CreateBuffer(&bufferDesc, NULL, &lightBuffer);

	// Create a texture sampler state description.
	waterSamplerDesc.Filter = D3D11_FILTER_ANISOTROPIC;
	waterSamplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	waterSamplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	waterSamplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	waterSamplerDesc.MipLODBias = 0.0f;
	waterSamplerDesc.MaxAnisotropy = 1;
	waterSamplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	waterSamplerDesc.MinLOD = 0;
	waterSamplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
	renderer->CreateSamplerState(&waterSamplerDesc, &waterSampleState);

	// Sampler for shadow map sampling.
	shadowSamplerDesc.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT; // for softer edges
	shadowSamplerDesc.ComparisonFunc = D3D11_COMPARISON_LESS_EQUAL; // handle shadow depth comparisons
	shadowSamplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
	shadowSamplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
	shadowSamplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
	shadowSamplerDesc.BorderColor[0] = 1.0f;
	shadowSamplerDesc.BorderColor[1] = 1.0f;
	shadowSamplerDesc.BorderColor[2] = 1.0f;
	shadowSamplerDesc.BorderColor[3] = 1.0f;
	renderer->CreateSamplerState(&shadowSamplerDesc, &sampleStateShadow);
	renderer->CreateSamplerState(&shadowSamplerDesc, &sampleStateShadow2);
}

void WaterShader::setShaderParameters(ID3D11DeviceContext* deviceContext, const XMMATRIX& worldMatrix, const XMMATRIX& viewMatrix, const XMMATRIX& projectionMatrix, ID3D11ShaderResourceView* water, ID3D11ShaderResourceView* depthMap, ID3D11ShaderResourceView* depthMap2, Camera* camera, Light* light, Light* directionalLight, SceneData* sceneData)
{
	HRESULT result;
	D3D11_MAPPED_SUBRESOURCE mappedResource;
	MatrixBufferType* dataPtr;
	XMMATRIX tworld, tview, tproj, tLightViewMatrix, tLightProjectionMatrix, tLightViewMatrix2, tLightProjectionMatrix2;

	// Transpose the matrices to prepare them for the shader.
	tworld = XMMatrixTranspose(worldMatrix);
	tview = XMMatrixTranspose(viewMatrix);
	tproj = XMMatrixTranspose(projectionMatrix);
	tLightViewMatrix = XMMatrixTranspose(light->getViewMatrix());
	tLightProjectionMatrix = XMMatrixTranspose(light->getOrthoMatrix());
	tLightViewMatrix2 = XMMatrixTranspose(directionalLight->getViewMatrix());
	tLightProjectionMatrix2 = XMMatrixTranspose(directionalLight->getOrthoMatrix());

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
	deviceContext->VSSetConstantBuffers(0, 1, &matrixBuffer);

	CameraBufferType* cameraPtr;
	deviceContext->Map(cameraBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	cameraPtr = (CameraBufferType*)mappedResource.pData;
	cameraPtr->cameraPosition = camera->getPosition();
	cameraPtr->padding = 0.0f;
	deviceContext->Unmap(cameraBuffer, 0);
	deviceContext->VSSetConstantBuffers(1, 1, &cameraBuffer);

	// Time and water waves info
	TimeBufferType* timePtr;
	deviceContext->Map(timeBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	timePtr = (TimeBufferType*)mappedResource.pData;
	timePtr->time = sceneData->waterData.timeVal;
	timePtr->amplitude = sceneData->waterData.amplitude;
	timePtr->frequency = sceneData->waterData.frequency;
	timePtr->speed = sceneData->waterData.speed;
	deviceContext->Unmap(timeBuffer, 0);
	deviceContext->VSSetConstantBuffers(2, 1, &timeBuffer);

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


	// Pixel Shader
	deviceContext->PSSetShaderResources(0, 1, &water);
	deviceContext->PSSetSamplers(0, 1, &waterSampleState);

	deviceContext->PSSetShaderResources(1, 1, &depthMap);
	deviceContext->PSSetSamplers(1, 1, &sampleStateShadow);

	deviceContext->PSSetShaderResources(2, 1, &depthMap2);
	deviceContext->PSSetSamplers(2, 1, &sampleStateShadow2);
}