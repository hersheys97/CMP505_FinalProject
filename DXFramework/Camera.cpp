// Camera class
// Represents a single 3D camera with basic movement.
#include "camera.h"

// Configure defaul camera (including positions, rotation and ortho matrix)
Camera::Camera()
{
	position = XMFLOAT3(0.f, 0.f, 0.f);
	rotation = XMFLOAT3(0.f, 0.f, 0.f);

	lookSpeed = 4.0f;

	// Generate ortho matrix
	XMVECTOR up, position, lookAt;
	up = XMVectorSet(0.0f, 1.0, 0.0, 1.0f);
	position = XMVectorSet(0.0f, 0.0, -10.0, 1.0f);
	lookAt = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0);
	orthoMatrix = XMMatrixLookAtLH(position, lookAt, up);
}

Camera::~Camera()
{
}

// Store frame/delta time.
void Camera::setFrameTime(float t)
{
	frameTime = t;
}

void Camera::setPosition(float lx, float ly, float lz)
{
	position.x = lx;
	position.y = ly;
	position.z = lz;
}

void Camera::setRotation(float lx, float ly, float lz)
{
	rotation.x = lx;
	rotation.y = ly;
	rotation.z = lz;
}

XMFLOAT3 Camera::getPosition()
{
	return position;
}

XMFLOAT3 Camera::getRotation() const
{
	return rotation;
}

// Re-calucation view Matrix.
void Camera::update()
{
	XMVECTOR up, positionv, lookAt;
	float yaw, pitch, roll;
	XMMATRIX rotationMatrix;

	// Setup the vectors
	up = XMVectorSet(0.0f, 1.0, 0.0, 1.0f);
	positionv = XMLoadFloat3(&position);
	lookAt = XMVectorSet(0.0, 0.0, 1.0f, 1.0f);

	// Set the yaw (Y axis), pitch (X axis), and roll (Z axis) rotations in radians.
	pitch = rotation.x * 0.0174532f;
	yaw = rotation.y * 0.0174532f;
	roll = rotation.z * 0.0174532f;

	// Create the rotation matrix from the yaw, pitch, and roll values.
	rotationMatrix = XMMatrixRotationRollPitchYaw(pitch, yaw, roll);

	// Transform the lookAt and up vector by the rotation matrix so the view is correctly rotated at the origin.
	lookAt = XMVector3TransformCoord(lookAt, rotationMatrix);
	up = XMVector3TransformCoord(up, rotationMatrix);

	// Translate the rotated camera position to the location of the viewer.
	lookAt = positionv + lookAt;

	// Finally create the view matrix from the three updated vectors.
	viewMatrix = XMMatrixLookAtLH(positionv, lookAt, up);
}


XMMATRIX Camera::getViewMatrix()
{
	return viewMatrix;
}

XMMATRIX Camera::getOrthoViewMatrix()
{
	return orthoMatrix;
}

void Camera::moveForward()
{
	float radians;

	// Update the forward movement based on the frame time
	speed = frameTime * 40.f;

	// Convert degrees to radians.
	radians = rotation.y * 0.0174532f;

	// Update the position.
	position.x += sinf(radians) * speed;
	position.z += cosf(radians) * speed;
}


void Camera::moveBackward()
{
	float radians;

	// Update the backward movement based on the frame time
	speed = frameTime * 40.f;// *0.5f;

	// Convert degrees to radians.
	radians = rotation.y * 0.0174532f;

	// Update the position.
	position.x -= sinf(radians) * speed;
	position.z -= cosf(radians) * speed;
}


void Camera::moveUpward()
{
	// Update the upward movement based on the frame time
	speed = frameTime * 40.f;// *0.5f;

	// Update the height position.
	position.y += speed;
}


void Camera::moveDownward()
{
	// Update the downward movement based on the frame time
	speed = frameTime * 40.f;// *0.5f;

	// Update the height position.
	position.y -= speed;
}


void Camera::turnLeft()
{
	// Update the left turn movement based on the frame time 
	speed = frameTime * 25.0f;

	// Update the rotation.
	rotation.y -= speed;

	// Keep the rotation in the 0 to 360 range.
	if (rotation.y < 0.0f)
	{
		rotation.y += 360.0f;
	}
}


void Camera::turnRight()
{
	// Update the right turn movement based on the frame time
	speed = frameTime * 25.0f;

	// Update the rotation.
	rotation.y += speed;

	// Keep the rotation in the 0 to 360 range.
	if (rotation.y > 360.0f)
	{
		rotation.y -= 360.0f;
	}

}


void Camera::turnUp()
{
	// Update the upward rotation movement based on the frame time
	speed = frameTime * 25.0f;

	// Update the rotation.
	rotation.x -= speed;

	// Keep the rotation maximum 90 degrees.
	if (rotation.x > 90.0f)
	{
		rotation.x = 90.0f;
	}
}


void Camera::turnDown()
{
	// Update the downward rotation movement based on the frame time
	speed = frameTime * 25.0f;

	// Update the rotation.
	rotation.x += speed;

	// Keep the rotation maximum 90 degrees.
	if (rotation.x < -90.0f)
	{
		rotation.x = -90.0f;
	}
}


void Camera::turn(int x, int y)
{
	// Update the rotation.
	rotation.y += (float)x / lookSpeed;// m_speed * x;

	rotation.x += (float)y / lookSpeed;// m_speed * y;
}

void Camera::strafeRight()
{
	float radians;

	// Update the forward movement based on the frame time
	speed = frameTime * 40.f;

	// Convert degrees to radians.
	radians = rotation.y * 0.0174532f;

	// Update the position.
	position.z -= sinf(radians) * speed;
	position.x += cosf(radians) * speed;

}

void Camera::strafeLeft()
{
	float radians;

	// Update the forward movement based on the frame time
	speed = frameTime * 40.f;

	// Convert degrees to radians.
	radians = rotation.y * 0.0174532f;

	// Update the position.
	position.z += sinf(radians) * speed;
	position.x -= cosf(radians) * speed;
}

XMFLOAT3 Camera::getForward() const {
	// Calculate forward vector from rotation
	float yaw = rotation.y * 0.0174532f;  // Convert to radians
	float pitch = rotation.x * 0.0174532f;

	XMFLOAT3 forward;
	forward.x = sinf(yaw) * cosf(pitch);
	forward.y = -sinf(pitch);  // Negative because positive pitch looks down
	forward.z = cosf(yaw) * cosf(pitch);

	// Normalize the vector
	XMVECTOR forwardVec = XMLoadFloat3(&forward);
	forwardVec = XMVector3Normalize(forwardVec);
	XMStoreFloat3(&forward, forwardVec);

	return forward;
}

XMFLOAT3 Camera::getUp() const {
	// The up vector is always world up (0,1,0) since we don't have roll
	return XMFLOAT3(0.0f, 1.0f, 0.0f);

	/* Alternative if you want the actual transformed up vector:
	XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	XMMATRIX rotationMatrix = XMMatrixRotationRollPitchYaw(
		rotation.x * 0.0174532f,
		rotation.y * 0.0174532f,
		rotation.z * 0.0174532f
	);
	up = XMVector3TransformNormal(up, rotationMatrix);

	XMFLOAT3 result;
	XMStoreFloat3(&result, up);
	return result;
	*/
}