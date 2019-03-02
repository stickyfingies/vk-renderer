#pragma once

#include <glm/glm.hpp>

struct CameraData
{
	alignas(16) glm::vec3 pos;
	alignas(16) glm::vec3 dir;

	alignas(16) glm::vec3 right;
	alignas(16) glm::vec3 up;
};

struct CameraDataAux
{
	glm::vec3 front;

	float pitch;
	float yaw;
};

struct Camera
{
	CameraData    data;
	CameraDataAux aux;

	Camera();

	void move_forward(float speed);

	void move_backward(float speed);

	void move_left(float speed);

	void move_right(float speed);

	void move_up(float speed);

	void move_down(float speed);


	void update();
};
