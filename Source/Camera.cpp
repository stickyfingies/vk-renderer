#include <Camera.h>

#include <algorithm>

Camera::Camera()
	: data{ .pos = glm::vec3(0.0f, 0.0f, 4.0f) }
	, aux{ .front = {glm::vec3(0.0f, 0.0f, -1.0f)} }
{
	update();
}

void Camera::move_forward(float speed)
{
	data.pos += aux.front * speed;
	update();
}

void Camera::move_backward(float speed)
{
	data.pos -= aux.front * speed;
	update();
}

void Camera::move_left(float speed)
{
	data.pos -= data.right * speed;
	update();
}

void Camera::move_right(float speed)
{
	data.pos += data.right * speed;
	update();
}

void Camera::move_up(float speed)
{
	data.pos += data.up * speed;
	update();
}

void Camera::move_down(float speed)
{
	data.pos -= data.up * speed;
	update();
}

void Camera::update()
{
	aux.pitch = std::clamp(aux.pitch, -89.0f, 89.0f);

	glm::vec3 front;

	front.x = cos(glm::radians(aux.pitch)) * cos(glm::radians(aux.yaw));
	front.y = sin(glm::radians(aux.pitch));
	front.z = cos(glm::radians(aux.pitch)) * sin(glm::radians(aux.yaw));

	aux.front = glm::normalize(front);

	data.dir   = glm::normalize((data.pos + aux.front) - data.pos);
	data.up    = glm::vec3(0.0f, 1.0f, 0.0f);
	data.right = glm::normalize(glm::cross(data.up, data.dir));
	data.up    = glm::cross(data.dir, data.right);
}
