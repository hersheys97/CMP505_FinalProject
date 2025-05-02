#include "EchoPulse.h"
#include <fstream>

EchoPulse::EchoPulse(ID3D11Device* device, HWND hwnd) : BaseShader(device, hwnd)
{
	initShader(L"echopulse_vs.cso", L"echopulse_ps.cso");
}

EchoPulse::~EchoPulse()
{
	// Release the sampler state.
	if (sampleState)
	{
		sampleState->Release();
		sampleState = 0;
	}

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

	//Release base shader components
	BaseShader::~BaseShader();
}

void EchoPulse::initShader(const wchar_t* vsFilename, const wchar_t* psFilename)
{
	D3D11_BUFFER_DESC matrixBufferDesc;
	D3D11_BUFFER_DESC pulseBufferDesc;
	D3D11_BUFFER_DESC heatDistortionBufferDesc;
	D3D11_SAMPLER_DESC samplerDesc;

	// Load (+ compile) shader files
	loadVertexShader(vsFilename);
	loadPixelShader(psFilename);

	// Setup the description of the dynamic matrix constant buffer that is in the vertex shader.
	matrixBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	matrixBufferDesc.ByteWidth = sizeof(MatrixBufferType);
	matrixBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	matrixBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	matrixBufferDesc.MiscFlags = 0;
	matrixBufferDesc.StructureByteStride = 0;
	renderer->CreateBuffer(&matrixBufferDesc, NULL, &matrixBuffer);

	pulseBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	pulseBufferDesc.ByteWidth = sizeof(PulseBufferType);
	pulseBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER; // This buffer will be bound to the pipeline as a constant buffer
	pulseBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE; // Allow CPU to write to the buffer
	pulseBufferDesc.MiscFlags = 0;
	pulseBufferDesc.StructureByteStride = 0;  // Not needed for constant buffers
	renderer->CreateBuffer(&pulseBufferDesc, NULL, &pulseBuffer);

	heatDistortionBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	heatDistortionBufferDesc.ByteWidth = sizeof(HeatDistortionBufferType);
	heatDistortionBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER; // This buffer will be bound to the pipeline as a constant buffer
	heatDistortionBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE; // Allow CPU to write to the buffer
	heatDistortionBufferDesc.MiscFlags = 0;
	heatDistortionBufferDesc.StructureByteStride = 0;  // Not needed for constant buffers
	renderer->CreateBuffer(&heatDistortionBufferDesc, NULL, &heatDistortionBuffer);

	// Create a texture sampler state description.
	samplerDesc.Filter = D3D11_FILTER_ANISOTROPIC;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.MipLODBias = 0.0f;
	samplerDesc.MaxAnisotropy = 1;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
	renderer->CreateSamplerState(&samplerDesc, &sampleState);
}

void EchoPulse::setShaderParameters(ID3D11DeviceContext* deviceContext, const XMMATRIX& worldMatrix, const XMMATRIX& viewMatrix, const XMMATRIX& projectionMatrix, ID3D11ShaderResourceView* texture, bool sonarActive, const XMFLOAT3 sonarOrigin, float sonarMaxRadius, float sonarTime, float screenWidth, float screenHeight, XMFLOAT3 cameraPosition, float timeVal, SceneData* sceneData)
{
	HRESULT result;
	D3D11_MAPPED_SUBRESOURCE mappedResource;

	XMMATRIX tworld, tview, tproj;
	// Transpose the matrices to prepare them for the shader.
	tworld = XMMatrixTranspose(worldMatrix);
	tview = XMMatrixTranspose(viewMatrix);
	tproj = XMMatrixTranspose(projectionMatrix);

	MatrixBufferType* dataPtr;
	if (FAILED(deviceContext->Map(matrixBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource))) OutputDebugStringA("Buffer creation failed!\n");

	deviceContext->Map(matrixBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	dataPtr = (MatrixBufferType*)mappedResource.pData;
	dataPtr->world = tworld;
	dataPtr->view = tview;
	dataPtr->projection = tproj;
	deviceContext->Unmap(matrixBuffer, 0);
	deviceContext->VSSetConstantBuffers(0, 1, &matrixBuffer);

	PulseBufferType* pulsePtr;
	deviceContext->Map(pulseBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	pulsePtr = (PulseBufferType*)mappedResource.pData;
	pulsePtr->sonarMaxRadius = sonarMaxRadius;
	pulsePtr->time = sonarTime;
	pulsePtr->screenWidth = screenWidth;
	pulsePtr->screenHeight = screenHeight;
	pulsePtr->sonarOrigin = sonarOrigin;
	pulsePtr->sonarActive = sonarActive;
	deviceContext->Unmap(pulseBuffer, 0);
	deviceContext->PSSetConstantBuffers(0, 1, &pulseBuffer);

	HeatDistortionBufferType* heatDistortionPtr;
	deviceContext->Map(heatDistortionBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	heatDistortionPtr = (HeatDistortionBufferType*)mappedResource.pData;
	heatDistortionPtr->distortionStrength = 0.2f; // 0.05 - 0.3
	heatDistortionPtr->distortionSpeed = 1.5f; // 0.5 - 3.0
	heatDistortionPtr->time = timeVal;
	heatDistortionPtr->padding = 0.0f;
	deviceContext->Unmap(heatDistortionBuffer, 0);
	deviceContext->PSSetConstantBuffers(1, 1, &heatDistortionBuffer);

	// Set shader textures
	deviceContext->PSSetShaderResources(0, 1, &texture);
	deviceContext->PSSetSamplers(0, 1, &sampleState);
}