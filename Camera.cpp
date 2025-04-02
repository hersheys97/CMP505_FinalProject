#include "pch.h"
#include "Camera.h"
#include <iostream> 

//camera for our app simple directX application. While it performs some basic functionality its incomplete. 
//

Camera::Camera()
{
	//initalise values. 
	//Orientation and Position are how we control the camera. 
	m_orientation.x = 0.0f;		//rotation around x - pitch
	m_orientation.y = 0.0f;		//rotation around y - yaw
	m_orientation.z = 0.0f;		//rotation around z - roll	//we tend to not use roll a lot in first person

	m_position.x = 0.0f;		//camera position in space. 
	m_position.y = 0.0f;
	m_position.z = 0.0f;

	//These variables are used for internal calculations and not set.  but we may want to queary what they 
	//externally at points
	m_lookat.x = 0.0f;		//Look target point
	m_lookat.y = 0.0f;
	m_lookat.z = 0.0f;

	m_forward.x = 0.0f;		//forward/look direction
	m_forward.y = 0.0f;
	m_forward.z = 0.0f;

	m_right.x = 0.0f;
	m_right.y = 0.0f;
	m_right.z = 0.0f;

	// Move speed
	m_movespeed = 1.2f;
	m_camRotRate = 12.0f;

	//force update with initial values to generate other camera data correctly for first update. 
	Update();
}


Camera::~Camera()
{
}

void Camera::Update()
{
	// Convert degrees to radians
	float pitch = DirectX::XMConvertToRadians(m_orientation.x); // Pitch (Up/Down)
	float yaw = DirectX::XMConvertToRadians(m_orientation.y);   // Yaw (Left/Right)

	// Calculate forward vector using both pitch and yaw
	m_forward.x = cos(pitch) * sin(yaw);
	m_forward.y = sin(pitch);  // Up/down movement
	m_forward.z = cos(pitch) * cos(yaw);
	m_forward.Normalize();

	// Create right vector
	m_forward.Cross(DirectX::SimpleMath::Vector3::UnitY, m_right);
	m_right.Normalize();

	// Recalculate up vector
	m_up = m_right.Cross(m_forward);
	m_up.Normalize();

	// Update lookat point
	m_lookat = m_position + m_forward;

	// Apply new camera matrix
	m_cameraMatrix = DirectX::SimpleMath::Matrix::CreateLookAt(m_position, m_lookat, m_up);
}

DirectX::SimpleMath::Matrix Camera::getCameraMatrix()
{
	return m_cameraMatrix;
}

void Camera::setPosition(DirectX::SimpleMath::Vector3 newPosition)
{
	m_position = newPosition;
}

DirectX::SimpleMath::Vector3 Camera::getPosition()
{
	return m_position;
}

DirectX::SimpleMath::Vector3 Camera::getForward()
{
	return m_forward;
}

void Camera::setRotation(DirectX::SimpleMath::Vector3 newRotation)
{
	m_orientation.x = std::max(-89.0f, std::min(89.0f, newRotation.x)); // Clamped to avoid flipping
	m_orientation.y = fmod(newRotation.y, 360.0f); // Wrap yaw to avoid huge values
	m_orientation.z = 0.0f;
}


DirectX::SimpleMath::Vector3 Camera::getRotation()
{
	return m_orientation;
}

float Camera::getMoveSpeed()
{
	return m_movespeed;
}

float Camera::getRotationSpeed()
{
	return m_camRotRate;
}

DirectX::SimpleMath::Vector3 Camera::getUp()
{
	// The up vector is always world up (UnitY) in this simple implementation
	return DirectX::SimpleMath::Vector3::UnitY;
}

DirectX::SimpleMath::Vector3 Camera::getRight()
{
	// Return the pre-calculated right vector
	return m_right;
}