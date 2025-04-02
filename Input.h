#pragma once
#include <DirectXMath.h>

struct InputCommands
{
	// Movement
	bool forward;
	bool back;
	bool right;
	bool left;
	bool up;        // Q key
	bool down;      // E key
	bool speedBoost; // Left Shift key

	// Rotation
	bool rotRight;  // Right arrow
	bool rotLeft;   // Left arrow
	bool rotUp;     // Up arrow
	bool rotDown;   // Down arrow

	bool generate;  // Space key

	// Mouse state
	bool mouseActive;
	bool mouseJustActivated; // Prevent abrupt rotation
	DirectX::XMINT2 mouseDelta;
	DirectX::XMINT2 lastMousePos;
};

class Input
{
public:
	Input();
	~Input();
	void Initialise(HWND window);
	void Update();
	bool Quit();
	InputCommands getGameInput();

	// Using old framework mouse logics
	void setMousePosition(int x, int y);
	void getMousePosition(int& x, int& y);
	void centerMouse();

private:
	bool m_quitApp;
	std::unique_ptr<DirectX::Keyboard> m_keyboard;
	std::unique_ptr<DirectX::Mouse> m_mouse;
	DirectX::Keyboard::KeyboardStateTracker m_KeyboardTracker;
	DirectX::Mouse::ButtonStateTracker m_MouseTracker;
	InputCommands m_GameInput;

	HWND m_window;
	int m_windowWidth;
	int m_windowHeight;
	POINT m_centerPosition;
};