#include "DomeShader.h"

DomeShader::DomeShader(ID3D11Device* device, HWND hwnd) : BaseShader(device, hwnd)
{
	initShader(L"dome_vs.cso", L"dome_ps.cso");
}

DomeShader::~DomeShader()
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

	if (domeSampleState)
	{
		domeSampleState->Release();
		domeSampleState = 0;
	}
	//Release base shader components
	BaseShader::~BaseShader();
}

void DomeShader::initShader(const wchar_t* vsFilename, const wchar_t* psFilename)
{
	D3D11_BUFFER_DESC matrixBufferDesc;
	D3D11_SAMPLER_DESC domeSampleDesc;

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

	// Create a texture sampler state description.
	domeSampleDesc.Filter = D3D11_FILTER_ANISOTROPIC;
	domeSampleDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	domeSampleDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	domeSampleDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	domeSampleDesc.MipLODBias = 0.0f;
	domeSampleDesc.MaxAnisotropy = 1;
	domeSampleDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	domeSampleDesc.MinLOD = 0;
	domeSampleDesc.MaxLOD = D3D11_FLOAT32_MAX;
	renderer->CreateSamplerState(&domeSampleDesc, &domeSampleState);
}

void DomeShader::setShaderParameters(ID3D11DeviceContext* deviceContext, const XMMATRIX& worldMatrix, const XMMATRIX& viewMatrix, const XMMATRIX& projectionMatrix, ID3D11ShaderResourceView* dome, SceneData* sceneData) {
	HRESULT result;
	D3D11_MAPPED_SUBRESOURCE mappedResource;
	MatrixBufferType* dataPtr;

	XMMATRIX tworld, tview, tproj;

	// Transpose the matrices to prepare them for the shader.
	tworld = XMMatrixTranspose(worldMatrix);
	tview = XMMatrixTranspose(viewMatrix);
	tproj = XMMatrixTranspose(projectionMatrix);
	result = deviceContext->Map(matrixBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	dataPtr = (MatrixBufferType*)mappedResource.pData;
	dataPtr->world = tworld;// worldMatrix;
	dataPtr->view = tview;
	dataPtr->projection = tproj;
	deviceContext->Unmap(matrixBuffer, 0);
	deviceContext->VSSetConstantBuffers(0, 1, &matrixBuffer);

	// Set shader texture resource in the pixel shader.
	deviceContext->PSSetShaderResources(0, 1, &dome);
	deviceContext->PSSetSamplers(0, 1, &domeSampleState);
}