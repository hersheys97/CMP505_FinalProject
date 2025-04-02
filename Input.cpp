#include "pch.h"
#include "Input.h"

Input::Input() : m_window(nullptr), m_windowWidth(0), m_windowHeight(0)
{
}

Input::~Input()
{
}

void Input::Initialise(HWND window)
{
	m_keyboard = std::make_unique<DirectX::Keyboard>();
	m_mouse = std::make_unique<DirectX::Mouse>();
	m_mouse->SetWindow(window);
	m_quitApp = false;
	m_window = window;

	// Get window dimensions
	RECT rect;
	GetClientRect(window, &rect);
	m_windowWidth = rect.right - rect.left;
	m_windowHeight = rect.bottom - rect.top;

	// Initialize input commands
	ZeroMemory(&m_GameInput, sizeof(InputCommands));
}

void Input::Update()
{
	auto kb = m_keyboard->GetState();
	m_KeyboardTracker.Update(kb);
	auto mouse = m_mouse->GetState();
	m_MouseTracker.Update(mouse);

	// Escape key to quit
	if (kb.Escape) m_quitApp = true;

	// Movement keys
	m_GameInput.forward = kb.W;
	m_GameInput.back = kb.S;
	m_GameInput.left = kb.A;
	m_GameInput.right = kb.D;
	m_GameInput.up = kb.E;
	m_GameInput.down = kb.Q;
	m_GameInput.speedBoost = kb.LeftShift;

	// Rotation keys
	m_GameInput.rotRight = kb.Right;
	m_GameInput.rotLeft = kb.Left;
	m_GameInput.rotUp = kb.Up;
	m_GameInput.rotDown = kb.Down;

	// Generate key
	m_GameInput.generate = kb.Space;

	// Mouse activation handling
	bool wasActive = m_GameInput.mouseActive;
	m_GameInput.mouseActive = mouse.rightButton;
	m_GameInput.mouseJustActivated = (!wasActive && m_GameInput.mouseActive);

	if (m_GameInput.mouseJustActivated)
	{
		// First activation - store current position and switch to relative mode
		/*m_GameInput.lastMousePos.x = mouse.x;
		m_GameInput.lastMousePos.y = mouse.y;*/
		m_mouse->SetMode(DirectX::Mouse::MODE_RELATIVE);
		ShowCursor(FALSE);
	}
	else if (!m_GameInput.mouseActive && wasActive)
	{
		// Deactivation
		m_mouse->SetMode(DirectX::Mouse::MODE_ABSOLUTE);
		ShowCursor(TRUE);
	}

	// Calculate mouse delta
	if (m_GameInput.mouseActive)
	{
		if (m_GameInput.mouseJustActivated)
		{
			// Reset mouse delta on activation to prevent sudden jumps
			m_GameInput.mouseDelta.x = 0;
			m_GameInput.mouseDelta.y = 0;
		}
		else
		{
			// Use the mouse's movement directly (no need to negate)
			m_GameInput.mouseDelta.x = -mouse.x; // Left/Right rotation
			m_GameInput.mouseDelta.y = -mouse.y; // Up/Down rotation
		}
	}
	else
	{
		m_GameInput.mouseDelta.x = 0;
		m_GameInput.mouseDelta.y = 0;
	}

}

bool Input::Quit()
{
	return m_quitApp;
}

InputCommands Input::getGameInput()
{
	return m_GameInput;
}