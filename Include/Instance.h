#pragma once

#define VK_USE_PLATFORM_XCB_KHR

#include <vulkan/vulkan.h>

class Instance
{
	VkInstance instance;

public:

	Instance();

	inline operator VkInstance () const { return instance; }

	void Create(bool debug);

	void Destroy();
};
