#include <Instance.h>

#include <cstring>
#include <iostream>
#include <vector>

Instance::Instance()
	: instance{VK_NULL_HANDLE}
{
	//
}

void Instance::Create(bool debug)
{
	VkApplicationInfo appInfo{VK_STRUCTURE_TYPE_APPLICATION_INFO};
	appInfo.pApplicationName   = "Square Demo";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName        = "VK Renderer";
	appInfo.engineVersion      = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion         = VK_API_VERSION_1_0;

	const char * extensionNames[]
	{
		VK_KHR_SURFACE_EXTENSION_NAME,
		VK_KHR_XCB_SURFACE_EXTENSION_NAME,
		VK_EXT_DEBUG_UTILS_EXTENSION_NAME
	};

	const char * layerNames[]
	{
		"VK_LAYER_LUNARG_standard_validation"
	};

	VkInstanceCreateInfo instanceInfo{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
	instanceInfo.pApplicationInfo        = &appInfo;
	instanceInfo.enabledExtensionCount   = sizeof(extensionNames) / sizeof(extensionNames[0]);
	instanceInfo.ppEnabledExtensionNames = extensionNames;

	if (debug)
	{
		instanceInfo.enabledLayerCount       = sizeof(layerNames) / sizeof(layerNames[0]);
		instanceInfo.ppEnabledLayerNames     = layerNames;
	}

	if (const auto res = vkCreateInstance(&instanceInfo, nullptr, &instance); res != VK_SUCCESS)
	{
		std::cout << "[vk] - err :: Failed to create instance :: " << res << std::endl;
		return;
	}
}

void Instance::Destroy()
{
	vkDestroyInstance(instance, nullptr);
}
