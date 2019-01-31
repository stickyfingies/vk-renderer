#include <Surface.h>

#include <GLFW/glfw3.h>

#include <iostream>

Surface::Surface(const Instance & instance)
	: instance{&instance}
{
	//
}

void Surface::Create(GLFWwindow * window)
{
	if (const auto res = glfwCreateWindowSurface(*instance, window, nullptr, &surface); res != VK_SUCCESS)
	{
		std::cout << "[vk] - err :: Failed to create surface :: " << res << std::endl;
		return;
	}
}
