#include "pch.h"
#include "Game.h"
#include <fstream> //toreorganise

extern void ExitGame();

using namespace DirectX;
using namespace DirectX::SimpleMath;
using namespace ImGui;

using Microsoft::WRL::ComPtr;

Game::Game() noexcept(false)
{
	m_deviceResources = std::make_unique<DX::DeviceResources>();
	m_deviceResources->RegisterDeviceNotify(this);
}

Game::~Game()
{
#ifdef DXTK_AUDIO
	if (m_audEngine) m_audEngine->Suspend();
#endif
}

// Initialize the Direct3D resources required to run.
void Game::Initialize(HWND window, int width, int height)
{
	m_input.Initialise(window);

	m_deviceResources->SetWindow(window, width, height);

	m_deviceResources->CreateDeviceResources();
	CreateDeviceDependentResources();

	m_deviceResources->CreateWindowSizeDependentResources();
	CreateWindowSizeDependentResources();

	//setup imgui.  its up here cos we need the window handle too
	//pulled from imgui directx11 example
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	ImGui::StyleColorsDark();
	ImGui_ImplWin32_Init(window);		//tie to our window
	ImGui_ImplDX11_Init(m_deviceResources->GetD3DDevice(), m_deviceResources->GetD3DDeviceContext());	//tie to directx

	m_fullscreenRect.left = 0;
	m_fullscreenRect.top = 0;
	m_fullscreenRect.right = 800;
	m_fullscreenRect.bottom = 600;

	m_CameraViewRect.left = 500;
	m_CameraViewRect.top = 0;
	m_CameraViewRect.right = 800;
	m_CameraViewRect.bottom = 240;

	//setup light
	m_Light.setAmbientColour(0.3f, 0.3f, 0.3f, 1.0f);
	m_Light.setDiffuseColour(1.0f, 1.0f, 1.0f, 1.0f);
	m_Light.setPosition(2.0f, 1.0f, 1.0f);
	m_Light.setDirection(-1.0f, -1.0f, 0.0f);

	//setup camera
	m_Camera01.setPosition(Vector3(0.0f, 0.0f, 4.0f));
	m_Camera01.setRotation(Vector3(0.0f, 0.0f, 0.0f));	//orientation is -90 becuase zero will be looking up at the sky straight up. 

#ifdef DXTK_AUDIO
	// Create DirectXTK for Audio objects
	AUDIO_ENGINE_FLAGS eflags = AudioEngine_Default;
#ifdef _DEBUG
	eflags = eflags | AudioEngine_Debug;
#endif

	m_audEngine = std::make_unique<AudioEngine>(eflags);

	m_audioEvent = 0;
	m_audioTimerAcc = 10.f;
	m_retryDefault = false;

	m_waveBank = std::make_unique<WaveBank>(m_audEngine.get(), L"adpcmdroid.xwb");

	m_soundEffect = std::make_unique<SoundEffect>(m_audEngine.get(), L"MusicMono_adpcm.wav");
	m_effect1 = m_soundEffect->CreateInstance();
	m_effect2 = m_waveBank->CreateInstance(10);

	m_effect1->Play(true);
	m_effect2->Play();
#endif
}

#pragma region Frame Update
// Executes the basic game loop.
void Game::Tick()
{
	//take in input
	m_input.Update();								//update the hardware
	m_gameInputCommands = m_input.getGameInput();	//retrieve the input for our game

	//Update all game objects
	m_timer.Tick([&]()
		{
			Update(m_timer);
		});

	//Render all game content. 
	Render();

#ifdef DXTK_AUDIO
	// Only update audio engine once per frame
	if (!m_audEngine->IsCriticalError() && m_audEngine->Update())
	{
		// Setup a retry in 1 second
		m_audioTimerAcc = 1.f;
		m_retryDefault = true;
	}
#endif
}

// Updates the world.
void Game::Update(DX::StepTimer const& timer)
{
	//this is hacky,  i dont like this here.  
	auto device = m_deviceResources->GetD3DDevice();

	float deltaTime = float(timer.GetElapsedSeconds());
	float moveSpeed = m_Camera01.getMoveSpeed() * deltaTime;

	// Unified rotation speed calculation
	float rotationSpeed = m_Camera01.getRotationSpeed() * deltaTime;
	float mouseRotationSpeed = rotationSpeed * m_Camera01.getMouseSensitivity();

	// Apply speed boost if left shift is pressed
	if (m_gameInputCommands.speedBoost)
	{
		moveSpeed *= 2.0f;
		rotationSpeed *= 2.0f;
		mouseRotationSpeed *= 2.0f;
	}

	// Handle camera movement
	if (m_gameInputCommands.forward)
	{
		Vector3 position = m_Camera01.getPosition();
		position += (m_Camera01.getForward() * moveSpeed);
		m_Camera01.setPosition(position);
	}
	if (m_gameInputCommands.back)
	{
		Vector3 position = m_Camera01.getPosition();
		position -= (m_Camera01.getForward() * moveSpeed);
		m_Camera01.setPosition(position);
	}
	if (m_gameInputCommands.left)
	{
		Vector3 position = m_Camera01.getPosition();
		position -= (m_Camera01.getRight() * moveSpeed);
		m_Camera01.setPosition(position);
	}
	if (m_gameInputCommands.right)
	{
		Vector3 position = m_Camera01.getPosition();
		position += (m_Camera01.getRight() * moveSpeed);
		m_Camera01.setPosition(position);
	}
	if (m_gameInputCommands.up)
	{
		Vector3 position = m_Camera01.getPosition();
		position += (m_Camera01.getUp() * moveSpeed);
		m_Camera01.setPosition(position);
	}
	if (m_gameInputCommands.down)
	{
		Vector3 position = m_Camera01.getPosition();
		position -= (m_Camera01.getUp() * moveSpeed);
		m_Camera01.setPosition(position);
	}

	// Handle mouse look
	if (m_gameInputCommands.mouseActive && !m_gameInputCommands.mouseJustActivated)
	{
		Vector3 rotation = m_Camera01.getRotation();

		// Apply mouse movement to rotation
		rotation.y += m_gameInputCommands.mouseDelta.x * mouseRotationSpeed;
		rotation.x += m_gameInputCommands.mouseDelta.y * mouseRotationSpeed;

		// Set rotation (clamping happens in setRotation)
		m_Camera01.setRotation(rotation);
	}

	if (m_gameInputCommands.generate)
	{
		m_Terrain.GenerateHeightMap(device);
	}

	m_Camera01.Update();	//camera update.
	m_Terrain.Update();		//terrain update.  doesnt do anything at the moment. 

	m_view = m_Camera01.getCameraMatrix();
	m_world = Matrix::Identity;

	/*create our UI*/
	SetupGUI();

#ifdef DXTK_AUDIO
	m_audioTimerAcc -= (float)timer.GetElapsedSeconds();
	if (m_audioTimerAcc < 0)
	{
		if (m_retryDefault)
		{
			m_retryDefault = false;
			if (m_audEngine->Reset())
			{
				// Restart looping audio
				m_effect1->Play(true);
			}
		}
		else
		{
			m_audioTimerAcc = 4.f;

			m_waveBank->Play(m_audioEvent++);

			if (m_audioEvent >= 11)
				m_audioEvent = 0;
		}
	}
#endif


	if (m_input.Quit())
	{
		ExitGame();
	}
}
#pragma endregion

#pragma region Frame Render
// Draws the scene.
void Game::Render()
{
	// Don't try to render anything before the first Update.
	if (m_timer.GetFrameCount() == 0)
	{
		return;
	}

	Clear();

	m_deviceResources->PIXBeginEvent(L"Render");
	auto context = m_deviceResources->GetD3DDeviceContext();
	auto renderTargetView = m_deviceResources->GetRenderTargetView();
	auto depthTargetView = m_deviceResources->GetDepthStencilView();

	// Draw Text to the screen
	m_sprites->Begin();
	m_font->DrawString(m_sprites.get(), L"Procedural Methods", XMFLOAT2(10, 10), Colors::Yellow);
	m_sprites->End();

	//Set Rendering states. 
	context->OMSetBlendState(m_states->Opaque(), nullptr, 0xFFFFFFFF);
	context->OMSetDepthStencilState(m_states->DepthDefault(), 0);
	context->RSSetState(m_states->CullClockwise());
	//	context->RSSetState(m_states->Wireframe());

		//prepare transform for floor object. 
	m_world = SimpleMath::Matrix::Identity; //set world back to identity
	SimpleMath::Matrix newPosition3 = SimpleMath::Matrix::CreateTranslation(0.0f, -0.6f, 0.0f);
	SimpleMath::Matrix newScale = SimpleMath::Matrix::CreateScale(0.1);		//scale the terrain down a little. 
	m_world = m_world * newScale * newPosition3;

	//setup and draw cube
	m_BasicShaderPair.EnableShader(context);
	m_BasicShaderPair.SetShaderParameters(context, &m_world, &m_view, &m_projection, &m_Light, m_texture1.Get());
	m_Terrain.Render(context);

	// Render the other objects?
	m_BasicModel.Render(context);
	//m_BasicModel2.Render(context);
	//m_BasicModel3.Render(context);

	//render our GUI
	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());


	// Show the new frame.
	m_deviceResources->Present();
}


// Helper method to clear the back buffers.
void Game::Clear()
{
	m_deviceResources->PIXBeginEvent(L"Clear");

	// Clear the views.
	auto context = m_deviceResources->GetD3DDeviceContext();
	auto renderTarget = m_deviceResources->GetRenderTargetView();
	auto depthStencil = m_deviceResources->GetDepthStencilView();

	context->ClearRenderTargetView(renderTarget, Colors::CornflowerBlue);
	context->ClearDepthStencilView(depthStencil, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
	context->OMSetRenderTargets(1, &renderTarget, depthStencil);

	// Set the viewport.
	auto viewport = m_deviceResources->GetScreenViewport();
	context->RSSetViewports(1, &viewport);

	m_deviceResources->PIXEndEvent();
}

#pragma endregion

#pragma region Message Handlers
// Message handlers
void Game::OnActivated()
{
}

void Game::OnDeactivated()
{
}

void Game::OnSuspending()
{
#ifdef DXTK_AUDIO
	m_audEngine->Suspend();
#endif
}

void Game::OnResuming()
{
	m_timer.ResetElapsedTime();

#ifdef DXTK_AUDIO
	m_audEngine->Resume();
#endif
}

void Game::OnWindowMoved()
{
	auto r = m_deviceResources->GetOutputSize();
	m_deviceResources->WindowSizeChanged(r.right, r.bottom);
}

void Game::OnWindowSizeChanged(int width, int height)
{
	if (!m_deviceResources->WindowSizeChanged(width, height))
		return;

	CreateWindowSizeDependentResources();
}

#ifdef DXTK_AUDIO
void Game::NewAudioDevice()
{
	if (m_audEngine && !m_audEngine->IsAudioDevicePresent())
	{
		// Setup a retry in 1 second
		m_audioTimerAcc = 1.f;
		m_retryDefault = true;
	}
}
#endif

// Properties
void Game::GetDefaultSize(int& width, int& height) const
{
	width = 800;
	height = 600;
}
#pragma endregion

#pragma region Direct3D Resources
// These are the resources that depend on the device.
void Game::CreateDeviceDependentResources()
{
	auto context = m_deviceResources->GetD3DDeviceContext();
	auto device = m_deviceResources->GetD3DDevice();

	m_states = std::make_unique<CommonStates>(device);
	m_fxFactory = std::make_unique<EffectFactory>(device);
	m_sprites = std::make_unique<SpriteBatch>(context);
	m_font = std::make_unique<SpriteFont>(device, L"SegoeUI_18.spritefont");
	m_batch = std::make_unique<PrimitiveBatch<VertexPositionColor>>(context);

	//setup our terrain - Planemesh
	m_Terrain.Initialize(device, 128, 128);

	//setup our test model
	m_BasicModel.InitializeSphere(device);
	m_BasicModel2.InitializeModel(device, "drone.obj");
	m_BasicModel3.InitializeBox(device, 10.0f, 50.f, 10.0f);	//box includes dimensions

	// Global shader, used on all the meshes. 
	m_BasicShaderPair.InitStandard(device, L"light_vs.cso", L"light_ps.cso");

	//load Textures
	CreateDDSTextureFromFile(device, L"seafloor.dds", nullptr, m_texture1.ReleaseAndGetAddressOf());
	CreateDDSTextureFromFile(device, L"EvilDrone_Diff.dds", nullptr, m_texture2.ReleaseAndGetAddressOf());

	//Initialise Render to texture
	m_FirstRenderPass = new RenderTexture(device, 800, 600, 1, 2);	//for our rendering, We dont use the last two properties. but.  they cant be zero and they cant be the same. 

}

// Allocate all memory resources that change on a window SizeChanged event.
void Game::CreateWindowSizeDependentResources()
{
	auto size = m_deviceResources->GetOutputSize();
	float aspectRatio = float(size.right) / float(size.bottom);
	float fovAngleY = 70.0f * XM_PI / 180.0f;

	// This is a simple example of change that can be made when the app is in
	// portrait or snapped view.
	if (aspectRatio < 1.0f)
	{
		fovAngleY *= 2.0f;
	}

	// This sample makes use of a right-handed coordinate system using row-major matrices.
	m_projection = Matrix::CreatePerspectiveFieldOfView(
		fovAngleY,
		aspectRatio,
		0.01f,
		100.0f
	);
}

void Game::SetupGUI()
{

	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	ImGui::Begin("Sin Wave Parameters");
	ImGui::SliderFloat("Wave Amplitude", m_Terrain.GetAmplitude(), 0.0f, 10.0f);
	ImGui::SliderFloat("Wavelength", m_Terrain.GetWavelength(), 0.0f, 1.0f);

	Vector3 rotation = m_Camera01.getRotation();
	float rotationXY[2] = { rotation.x, rotation.y };
	ImGui::SliderFloat2("Camera X, Y", rotationXY, -180.0f, 180.0f);
	ImGui::End();
}

void Game::OnDeviceLost()
{
	m_states.reset();
	m_fxFactory.reset();
	m_sprites.reset();
	m_font.reset();
	m_batch.reset();
	m_testmodel.reset();
	m_batchInputLayout.Reset();
}

void Game::OnDeviceRestored()
{
	CreateDeviceDependentResources();
	CreateWindowSizeDependentResources();
}
#pragma endregion