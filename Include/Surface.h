#pragma once

#include <Instance.h>

#include <vulkan/vulkan.h>

struct GLFWwindow;

class Surface
{
	VkSurfaceKHR surface;

	const Instance * instance;

public:

	Surface(const Instance & instance);

	void Create(GLFWwindow * window);

	void Destroy();
};
