#include "ChromaticAberration.h"
#include <fstream>

ChromaticAberration::ChromaticAberration(ID3D11Device* device, HWND hwnd) : BaseShader(device, hwnd)
{
	initShader(L"aberration_vs.cso", L"aberration_ps.cso");
}

ChromaticAberration::~ChromaticAberration()
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

void ChromaticAberration::initShader(const wchar_t* vsFilename, const wchar_t* psFilename)
{
	D3D11_BUFFER_DESC matrixBufferDesc;
	D3D11_BUFFER_DESC aberrationBufferDesc;
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

	aberrationBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	aberrationBufferDesc.ByteWidth = sizeof(AberrationBufferType);
	aberrationBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER; // This buffer will be bound to the pipeline as a constant buffer
	aberrationBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE; // Allow CPU to write to the buffer
	aberrationBufferDesc.MiscFlags = 0;
	aberrationBufferDesc.StructureByteStride = 0;  // Not needed for constant buffers
	renderer->CreateBuffer(&aberrationBufferDesc, NULL, &aberrationBuffer);

	// Create a texture sampler state description.
	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
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

void ChromaticAberration::setShaderParameters(ID3D11DeviceContext* deviceContext, const XMMATRIX& worldMatrix, const XMMATRIX& orthoView, const XMMATRIX& orthoMatrix, ID3D11ShaderResourceView* texture, XMFLOAT2 offsets, XMFLOAT2 ghostScreenPos, float ghostDistance, float timeVal, float effectIntensity)
{
	HRESULT result;
	D3D11_MAPPED_SUBRESOURCE mappedResource;

	XMMATRIX tworld, tview, tproj;
	// Transpose the matrices to prepare them for the shader.
	tworld = XMMatrixTranspose(worldMatrix);
	tview = XMMatrixTranspose(orthoView);
	tproj = XMMatrixTranspose(orthoMatrix);

	MatrixBufferType* dataPtr;
	deviceContext->Map(matrixBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	dataPtr = (MatrixBufferType*)mappedResource.pData;
	dataPtr->world = tworld;
	dataPtr->view = tview;
	dataPtr->projection = tproj;
	deviceContext->Unmap(matrixBuffer, 0);
	deviceContext->VSSetConstantBuffers(0, 1, &matrixBuffer);

	AberrationBufferType* aberPtr;
	deviceContext->Map(aberrationBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	aberPtr = (AberrationBufferType*)mappedResource.pData;
	aberPtr->chromaticOffset = offsets;
	aberPtr->ghostScreenPos = ghostScreenPos;
	aberPtr->ghostDistance = ghostDistance;
	aberPtr->timeVal = timeVal;
	aberPtr->effectIntensity = effectIntensity;
	aberPtr->padding = 0.f;
	deviceContext->Unmap(aberrationBuffer, 0);
	deviceContext->PSSetConstantBuffers(1, 1, &aberrationBuffer);

	// Set shader textures
	deviceContext->PSSetShaderResources(0, 1, &texture);
	deviceContext->PSSetSamplers(0, 1, &sampleState);
}