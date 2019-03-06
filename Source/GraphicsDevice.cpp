#include <GraphicsDevice.h>
#include "VulkanState.h"

#include <GLFW/glfw3.h>

#include <algorithm>
#include <cstring>
#include <fstream>
#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <vector>

struct ImageView_T
{
	VkImageView handle;
};

static std::vector<char> ReadFile(const char * filename)
{
	std::ifstream file(filename, std::ios::ate | std::ios::binary);

	if (file.is_open() == false)
	{
		std::cout << "[app] - err :: Failed to open file" << std::endl;
		return {};
	}

	size_t fileSize = (size_t) file.tellg();
	std::vector<char> buffer(fileSize);

	file.seekg(0);
	file.read(buffer.data(), fileSize);

	file.close();

	return buffer;
}

namespace
{
	VulkanState state{};

	const VkFormat imageFormatTable[]
	{
		VK_FORMAT_UNDEFINED,
		VK_FORMAT_B8G8R8A8_UNORM
	};

	const VkImageLayout imageLayoutTable[]
	{
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
	};

	const VkAttachmentLoadOp loadOpTable[]
	{
		VK_ATTACHMENT_LOAD_OP_CLEAR
	};

	const VkAttachmentStoreOp storeOpTable[]
	{
		VK_ATTACHMENT_STORE_OP_STORE
	};

	const VkShaderStageFlagBits shaderStageTable[]
	{
		VK_SHADER_STAGE_VERTEX_BIT,
		VK_SHADER_STAGE_FRAGMENT_BIT
	};
}

uint32_t find_memory_type(uint32_t type_filter, VkMemoryPropertyFlags properties)
{
	VkPhysicalDeviceMemoryProperties mem_properties;
	vkGetPhysicalDeviceMemoryProperties(state.physicalDevice, &mem_properties);

	for (unsigned int i = 0; i < mem_properties.memoryTypeCount; ++i)
	{
		if ((type_filter & (1 << i)) && (mem_properties.memoryTypes[i].propertyFlags & properties) == properties)
		{
			return i;
		}
	}

	std::cout << "Failed to find suitable memory type" << std::endl;
	return 0;
}

GraphicsDevice::Error GraphicsDevice::Construct(const GraphicsDevice::CreateInfo & info)
{
	// Create instance
	{
		state.FRAMES_IN_FLIGHT    = info.framesInFlight;
		state.RAYTRACE_RESOLUTION = info.raytrace_resolution;

		VkApplicationInfo appInfo{VK_STRUCTURE_TYPE_APPLICATION_INFO};
		appInfo.pApplicationName   = "Square Demo";
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName        = "VK Render Backend";
		appInfo.engineVersion      = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion         = VK_API_VERSION_1_0;

		const char * extensionNames[]
		{
			VK_KHR_SURFACE_EXTENSION_NAME,
			VK_KHR_XCB_SURFACE_EXTENSION_NAME,
			VK_EXT_DEBUG_UTILS_EXTENSION_NAME
		};

		VkInstanceCreateInfo instanceInfo{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
		instanceInfo.pApplicationInfo        = &appInfo;
		instanceInfo.enabledExtensionCount   = sizeof(extensionNames) / sizeof(extensionNames[0]);
		instanceInfo.ppEnabledExtensionNames = extensionNames;

		if (info.debug)
		{
			const char * layerNames[]
			{
				"VK_LAYER_LUNARG_standard_validation"
			};

			instanceInfo.enabledLayerCount   = sizeof(layerNames) / sizeof(layerNames[0]);
			instanceInfo.ppEnabledLayerNames = layerNames;
		}

		if (vkCreateInstance(&instanceInfo, nullptr, &state.instance) != VK_SUCCESS)
		{
			return Error::UNKNOWN;
		}
	}

	// Create surface
	{
		if (glfwCreateWindowSurface(state.instance, info.window, nullptr, &state.surface) != VK_SUCCESS)
		{
			return Error::UNKNOWN;
		}
	}

	// Choose physical device
	{
		unsigned int availableDeviceCount = 0;
		vkEnumeratePhysicalDevices(state.instance, &availableDeviceCount, nullptr);

		if (availableDeviceCount == 0)
		{
			// We weren't able to find any GPUs on the system
			return Error::NO_SUITABLE_GPU;
		}

		std::vector<VkPhysicalDevice> availableDevices{availableDeviceCount};
		vkEnumeratePhysicalDevices(state.instance, &availableDeviceCount, availableDevices.data());

		const char * requiredExtensions[]
		{
			VK_KHR_SWAPCHAIN_EXTENSION_NAME
		};

		// Attempt to find suitable GPU

		for (const auto & device : availableDevices)
		{
			unsigned int supportedExtensionCount = 0;

			// Enumerate available GPU extensions

			uint32_t availableExtensionCount;
			vkEnumerateDeviceExtensionProperties(device, nullptr, &availableExtensionCount, nullptr);
			std::vector<VkExtensionProperties> availableExtensions{availableExtensionCount};
			vkEnumerateDeviceExtensionProperties(device, nullptr, &availableExtensionCount, availableExtensions.data());

			// Check if GPU supports all of our required extensions

			for (const auto & requiredExtension : requiredExtensions)
			{
				for (const auto & availableExtension : availableExtensions)
				{
					if (strcmp(requiredExtension, availableExtension.extensionName) == 0)
					{
						++supportedExtensionCount;
						break;
					}
				}
			}

			if (supportedExtensionCount != (sizeof(requiredExtensions) / sizeof(requiredExtensions[0])))
			{
				// GPU doesn't support all of our required extensions
				continue;
			}

			// At this point all checks have passed for the current device

			state.physicalDevice = device;
		}

		if (state.physicalDevice == VK_NULL_HANDLE)
		{
			// No suitable GPU was found on the system
			return Error::NO_SUITABLE_GPU;
		}
	}

	// Create logical device
	{
		// Enumerate available GPU queue families

		unsigned int queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(state.physicalDevice, &queueFamilyCount, nullptr);
		std::vector<VkQueueFamilyProperties> queueFamilies{queueFamilyCount};
		vkGetPhysicalDeviceQueueFamilyProperties(state.physicalDevice, &queueFamilyCount, queueFamilies.data());

		// Retrieve queues from queue families

		int queueIndex = 0;
		for (const auto & queueFamily : queueFamilies)
		{
			if (queueFamily.queueCount == 0)
			{
				// Queue family does not contain any queues
				continue;
			}

			if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
			{
				state.graphicsQueueIndex = queueIndex;
			}

			VkBool32 presentSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(state.physicalDevice, queueIndex, state.surface, &presentSupport);

			if (presentSupport)
			{
				state.presentQueueIndex = queueIndex;
			}

			if (state.graphicsQueueIndex && state.presentQueueIndex)
			{
				// All queues have been found; stop searching
				break;
			}

			++queueIndex;
		}

		std::vector<int> uniqueQueues;
		uniqueQueues.push_back(state.graphicsQueueIndex);
		uniqueQueues.push_back(state.presentQueueIndex);

		uniqueQueues.erase(std::unique(uniqueQueues.begin(), uniqueQueues.end()), uniqueQueues.end());

		std::vector<VkDeviceQueueCreateInfo> queueInfos;

		const float queuePriority = 1.0f;

		for (const auto & queue : uniqueQueues)
		{
			VkDeviceQueueCreateInfo queueCreateInfo{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};

			queueCreateInfo.queueFamilyIndex = queue;
			queueCreateInfo.queueCount       = 1;
			queueCreateInfo.pQueuePriorities = &queuePriority;

			queueInfos.push_back(queueCreateInfo);
		}

		const char * requiredExtensions[]
		{
			VK_KHR_SWAPCHAIN_EXTENSION_NAME
		};

		VkPhysicalDeviceFeatures deviceFeatures{};

		VkDeviceCreateInfo createInfo{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};

		createInfo.queueCreateInfoCount = queueInfos.size();

		createInfo.pQueueCreateInfos = queueInfos.data();
		createInfo.pEnabledFeatures  = &deviceFeatures;

		createInfo.enabledExtensionCount   = sizeof(requiredExtensions) / sizeof(requiredExtensions[0]);
		createInfo.ppEnabledExtensionNames = requiredExtensions;

		if (info.debug)
		{
			const char * layerNames[]
			{
				"VK_LAYER_LUNARG_standard_validation"
			};

			createInfo.enabledLayerCount   = sizeof(layerNames) / sizeof(layerNames[0]);
			createInfo.ppEnabledLayerNames = layerNames;
		}

		if (vkCreateDevice(state.physicalDevice, &createInfo, nullptr, &state.device) != VK_SUCCESS)
		{
			return Error::UNKNOWN;
		}

		vkGetDeviceQueue(state.device, state.graphicsQueueIndex, 0, &state.graphicsQueue);

		vkGetDeviceQueue(state.device, state.presentQueueIndex, 0, &state.presentQueue);
	}

	// Create command pool / buffers
	{
		VkCommandPoolCreateInfo poolInfo{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};

		poolInfo.queueFamilyIndex = state.graphicsQueueIndex;
		poolInfo.flags = 0;

		VkCommandBufferAllocateInfo allocInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};

		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = 1;

		state.commandPools.resize(state.FRAMES_IN_FLIGHT);
		state.commandBuffers.resize(state.FRAMES_IN_FLIGHT);

		for (unsigned char i = 0; i < state.FRAMES_IN_FLIGHT; ++i)
		{
			if (vkCreateCommandPool(state.device, &poolInfo, nullptr, &state.commandPools[i]) != VK_SUCCESS)
			{
				std::cout << "[app] - err :: Failed to create command pool" << std::endl;
				return Error::UNKNOWN;
			}

			allocInfo.commandPool = state.commandPools[i];

			if (vkAllocateCommandBuffers(state.device, &allocInfo, &state.commandBuffers[i]) != VK_SUCCESS)
			{
				std::cout << "[app] - err :: Failed to create command buffers" << std::endl;
				return Error::UNKNOWN;
			}
		}
	}

	// Create swapchain
	{
		// Enumerate available surface formats

		unsigned int availableFormatCount;
		vkGetPhysicalDeviceSurfaceFormatsKHR(state.physicalDevice, state.surface, &availableFormatCount, nullptr);

		if (availableFormatCount == 0)
		{
			// Surface does not avail any supported pixel formats
			return Error::NO_SUITABLE_SURFACE;
		}

		std::vector<VkSurfaceFormatKHR> availableFormats{availableFormatCount};
		vkGetPhysicalDeviceSurfaceFormatsKHR(state.physicalDevice, state.surface, &availableFormatCount, availableFormats.data());

		// Choose optimal surface format

		if (availableFormats.size() == 1 && availableFormats[0].format == VK_FORMAT_UNDEFINED)
		{
			state.swapchain.surfaceFormat = {VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
		}

		for (const auto & availableFormat : availableFormats)
		{
			if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			{
				state.swapchain.surfaceFormat = availableFormat;
			}
		}

		// Enumerate available present modes

		unsigned int availablePresentModeCount;
		vkGetPhysicalDeviceSurfacePresentModesKHR(state.physicalDevice, state.surface, &availablePresentModeCount, nullptr);

		if (availablePresentModeCount == 0)
		{
			// Surface does not avail any supported present modes
			return Error::NO_SUITABLE_SURFACE;
		}

		std::vector<VkPresentModeKHR> availablePresentModes{availablePresentModeCount};
		vkGetPhysicalDeviceSurfacePresentModesKHR(state.physicalDevice, state.surface, &availablePresentModeCount, availablePresentModes.data());

		// Choose optimal present mode

		VkPresentModeKHR bestMode = VK_PRESENT_MODE_FIFO_KHR;

		for (const auto & presentMode : availablePresentModes)
		{
			if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR)
			{
				state.swapchain.presentMode = presentMode;
			}
			else if (presentMode == VK_PRESENT_MODE_IMMEDIATE_KHR)
			{
				state.swapchain.presentMode = presentMode;
			}
		}

		// Enumerate available swap extents

		VkSurfaceCapabilitiesKHR surfaceCapabilities;
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(state.physicalDevice, state.surface, &surfaceCapabilities);

		if (surfaceCapabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
		{
			state.swapchain.extent = surfaceCapabilities.currentExtent;
		}
		else
		{
			int width;
			int height;
			glfwGetFramebufferSize(info.window, &width, &height);

			VkExtent2D actualExtent{static_cast<uint32_t>(width), static_cast<uint32_t>(height)};

			actualExtent.width  = std::clamp(actualExtent.width, surfaceCapabilities.minImageExtent.width, surfaceCapabilities.maxImageExtent.width);
			actualExtent.height = std::clamp(actualExtent.height, surfaceCapabilities.minImageExtent.height, surfaceCapabilities.maxImageExtent.height);

			state.swapchain.extent = actualExtent;
		}

		// Create swapchain

		unsigned int imageCount = info.swapchainSize == 0 ? surfaceCapabilities.minImageCount + 1 : info.swapchainSize;

		if (info.swapchainSize != 0 && surfaceCapabilities.minImageCount > info.swapchainSize)
		{
			imageCount = surfaceCapabilities.minImageCount;
		}

		if (surfaceCapabilities.maxImageCount > 0 && imageCount > surfaceCapabilities.maxImageCount)
		{
			imageCount = surfaceCapabilities.maxImageCount;
		}

		VkSwapchainCreateInfoKHR createInfo{VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
		createInfo.surface          = state.surface;
		createInfo.minImageCount    = imageCount;
		createInfo.imageFormat      = state.swapchain.surfaceFormat.format;
		createInfo.imageColorSpace  = state.swapchain.surfaceFormat.colorSpace;
		createInfo.imageExtent      = state.swapchain.extent;
		createInfo.imageArrayLayers = 1;
		createInfo.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		createInfo.preTransform     = surfaceCapabilities.currentTransform;
		createInfo.compositeAlpha   = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		createInfo.presentMode      = state.swapchain.presentMode;
		createInfo.clipped          = VK_TRUE;

		if (vkCreateSwapchainKHR(state.device, &createInfo, nullptr, &state.swapchain.swapchain) != VK_SUCCESS)
		{
			return Error::NO_SUITABLE_SURFACE;
		}

		// Retrieve swapchain images

		vkGetSwapchainImagesKHR(state.device, state.swapchain.swapchain, &imageCount, nullptr);
		state.swapchain.images.resize(imageCount);
		vkGetSwapchainImagesKHR(state.device, state.swapchain.swapchain, &imageCount, state.swapchain.images.data());

		// Create image views

		state.swapchain.imageViews.resize(state.swapchain.images.size());

		for (unsigned int i = 0; i < state.swapchain.imageViews.size(); ++i)
		{
			VkImageViewCreateInfo createInfo{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};

			createInfo.image    = state.swapchain.images[i];
			createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			createInfo.format   = state.swapchain.surfaceFormat.format;

			createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

			createInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
			createInfo.subresourceRange.baseMipLevel   = 0;
			createInfo.subresourceRange.levelCount     = 1;
			createInfo.subresourceRange.baseArrayLayer = 0;
			createInfo.subresourceRange.layerCount     = 1;

			if (vkCreateImageView(state.device, &createInfo, nullptr, &state.swapchain.imageViews[i]) != VK_SUCCESS)
			{
				return Error::NO_SUITABLE_SURFACE;
			}
		}

		// Create semaphores

		state.swapchain.imageAvailableSemaphores.resize(info.framesInFlight);
		state.swapchain.renderFinishedSemaphores.resize(info.framesInFlight);

		state.swapchain.frameFences.resize(info.framesInFlight);

		for (unsigned char i = 0; i < info.framesInFlight; ++i)
		{
			VkSemaphoreCreateInfo semaphoreInfo{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};

			VkFenceCreateInfo fenceInfo{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};

			fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

			if (vkCreateSemaphore(state.device, &semaphoreInfo, nullptr, &state.swapchain.imageAvailableSemaphores[i]) != VK_SUCCESS
				|| vkCreateSemaphore(state.device, &semaphoreInfo, nullptr, &state.swapchain.renderFinishedSemaphores[i]) != VK_SUCCESS)
			{
				std::cout << "[app] - err :: Failed to create swapchain semaphores" << std::endl;
				return Error::UNKNOWN;
			}

			if (vkCreateFence(state.device, &fenceInfo, nullptr, &state.swapchain.frameFences[i]) != VK_SUCCESS)
			{
				std::cout << "[app] - err :: Failed to create swapchain fence" << std::endl;
			}
		}
	}

	// Create render pass
	{
		VkAttachmentDescription backbuffer_desc{};

		backbuffer_desc.format  = state.swapchain.surfaceFormat.format;
		backbuffer_desc.samples = VK_SAMPLE_COUNT_1_BIT;

		backbuffer_desc.loadOp  = VK_ATTACHMENT_LOAD_OP_CLEAR;
		backbuffer_desc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

		backbuffer_desc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		backbuffer_desc.finalLayout   = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentReference backbuffer_reference{};

		backbuffer_reference.attachment = 0;

		backbuffer_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass_desc{};

		subpass_desc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

		subpass_desc.colorAttachmentCount = 1;
		subpass_desc.pColorAttachments    = &backbuffer_reference;

		VkRenderPassCreateInfo render_pass_info{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};

		render_pass_info.attachmentCount = 1;
		render_pass_info.pAttachments    = &backbuffer_desc;

		render_pass_info.subpassCount = 1;
		render_pass_info.pSubpasses   = &subpass_desc;

		vkCreateRenderPass(state.device, &render_pass_info, nullptr, &state.render_pass);
	}

	// Create framebuffers
	{
		state.swapchain.framebuffers.resize(state.swapchain.imageViews.size());

		for (unsigned int i = 0; i < state.swapchain.imageViews.size(); ++i)
		{
			VkFramebufferCreateInfo framebufferInfo{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};

			framebufferInfo.renderPass = state.render_pass;

			framebufferInfo.attachmentCount = 1;
			framebufferInfo.pAttachments    = &state.swapchain.imageViews[i];

			framebufferInfo.width  = state.swapchain.extent.width;
			framebufferInfo.height = state.swapchain.extent.height;
			framebufferInfo.layers = 1;

			vkCreateFramebuffer(state.device, &framebufferInfo, nullptr, &state.swapchain.framebuffers[i]);
		}
	}

	// Create shader resources
	{
		VkImageCreateInfo raytrace_image_info{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};

		raytrace_image_info.imageType = VK_IMAGE_TYPE_2D;
		raytrace_image_info.format    = VK_FORMAT_R8G8B8A8_UNORM;
		raytrace_image_info.tiling    = VK_IMAGE_TILING_OPTIMAL;
		raytrace_image_info.usage     = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
		raytrace_image_info.samples   = VK_SAMPLE_COUNT_1_BIT;

		raytrace_image_info.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
		raytrace_image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

		raytrace_image_info.extent.width  = static_cast<uint32_t>(state.RAYTRACE_RESOLUTION);
		raytrace_image_info.extent.height = static_cast<uint32_t>(state.RAYTRACE_RESOLUTION);
		raytrace_image_info.extent.depth  = 1;

		raytrace_image_info.mipLevels   = 1;
		raytrace_image_info.arrayLayers = 1;

		vkCreateImage(state.device, &raytrace_image_info, nullptr, &state.raytrace_storage_image);

		VkMemoryRequirements raytrace_image_mem_reqs;
		vkGetImageMemoryRequirements(state.device, state.raytrace_storage_image, &raytrace_image_mem_reqs);

		VkMemoryAllocateInfo raytrace_image_alloc_info{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
		raytrace_image_alloc_info.allocationSize  = raytrace_image_mem_reqs.size;
		raytrace_image_alloc_info.memoryTypeIndex = find_memory_type(raytrace_image_mem_reqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		vkAllocateMemory(state.device, &raytrace_image_alloc_info, nullptr, &state.raytrace_storage_image_memory);

		vkBindImageMemory(state.device, state.raytrace_storage_image, state.raytrace_storage_image_memory, 0);

		VkCommandBufferAllocateInfo allocInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = state.commandPools[0];
		allocInfo.commandBufferCount = 1;

		VkCommandBuffer commandBuffer;
		vkAllocateCommandBuffers(state.device, &allocInfo, &commandBuffer);

		VkCommandBufferBeginInfo beginInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		vkBeginCommandBuffer(commandBuffer, &beginInfo);

		{
			VkImageMemoryBarrier barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
			barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.image = state.raytrace_storage_image;
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			barrier.subresourceRange.baseMipLevel = 0;
			barrier.subresourceRange.levelCount = 1;
			barrier.subresourceRange.baseArrayLayer = 0;
			barrier.subresourceRange.layerCount = 1;
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			vkCmdPipelineBarrier(
				commandBuffer,
				VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
				0,
				0, nullptr,
				0, nullptr,
				1, &barrier
			);
		}

		vkEndCommandBuffer(commandBuffer);

		VkSubmitInfo submitInfo{VK_STRUCTURE_TYPE_SUBMIT_INFO};
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers    = &commandBuffer;

		vkQueueSubmit(state.graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
		vkQueueWaitIdle(state.graphicsQueue);

		vkFreeCommandBuffers(state.device, state.commandPools[0], 1, &commandBuffer);

		VkImageViewCreateInfo raytrace_image_view_info{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};

		raytrace_image_view_info.image    = state.raytrace_storage_image;
		raytrace_image_view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
		raytrace_image_view_info.format   = VK_FORMAT_R8G8B8A8_UNORM;

		raytrace_image_view_info.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
		raytrace_image_view_info.subresourceRange.baseMipLevel   = 0;
		raytrace_image_view_info.subresourceRange.levelCount     = 1;
		raytrace_image_view_info.subresourceRange.baseArrayLayer = 0;
		raytrace_image_view_info.subresourceRange.layerCount     = 1;

		vkCreateImageView(state.device, &raytrace_image_view_info, nullptr, &state.raytrace_storage_image_view);

		VkSamplerCreateInfo raytrace_image_sampler_info{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};

		raytrace_image_sampler_info.magFilter = VK_FILTER_LINEAR;
		raytrace_image_sampler_info.minFilter = VK_FILTER_LINEAR;

		raytrace_image_sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
		raytrace_image_sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
		raytrace_image_sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;

		raytrace_image_sampler_info.anisotropyEnable = VK_FALSE;
		raytrace_image_sampler_info.maxAnisotropy    = 1;

		raytrace_image_sampler_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;

		raytrace_image_sampler_info.unnormalizedCoordinates = VK_FALSE;

		raytrace_image_sampler_info.compareEnable = VK_FALSE;
		raytrace_image_sampler_info.compareOp     = VK_COMPARE_OP_ALWAYS;

		raytrace_image_sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		raytrace_image_sampler_info.mipLodBias = 0.0f;
		raytrace_image_sampler_info.minLod     = 0.0f;
		raytrace_image_sampler_info.maxLod     = 0.0f;

		vkCreateSampler(state.device, &raytrace_image_sampler_info, nullptr, &state.raytrace_storage_image_sampler);
	}

	// Create graphics descriptors
	{
		VkDescriptorSetLayoutBinding blit_sampler_binding{};

		blit_sampler_binding.binding    = 0;
		blit_sampler_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		blit_sampler_binding.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		blit_sampler_binding.descriptorCount = 1;

		VkDescriptorSetLayoutCreateInfo layout_info{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
		layout_info.bindingCount = 1;
		layout_info.pBindings    = &blit_sampler_binding;

		vkCreateDescriptorSetLayout(state.device, &layout_info, nullptr, &state.graphics_descset_layout);

		VkDescriptorPoolSize pool_size{};

		pool_size.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

		pool_size.descriptorCount = static_cast<uint32_t>(state.swapchain.imageViews.size());

		VkDescriptorPoolCreateInfo pool_info{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};

		pool_info.poolSizeCount = 1;
		pool_info.pPoolSizes    = &pool_size;
		pool_info.maxSets       = static_cast<uint32_t>(state.swapchain.imageViews.size());

		vkCreateDescriptorPool(state.device, &pool_info, nullptr, &state.graphics_desc_pool);

		VkDescriptorSetAllocateInfo alloc_info{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
		alloc_info.descriptorPool     = state.graphics_desc_pool;
		alloc_info.descriptorSetCount = 1;
		alloc_info.pSetLayouts        = &state.graphics_descset_layout;

		vkAllocateDescriptorSets(state.device, &alloc_info, &state.graphics_descset);

		VkDescriptorImageInfo image_info{};

		image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		image_info.imageView = state.raytrace_storage_image_view;
		image_info.sampler   = state.raytrace_storage_image_sampler;

		VkWriteDescriptorSet descriptor_write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};

		descriptor_write.dstSet = state.graphics_descset;
		descriptor_write.dstBinding = 0;
		descriptor_write.dstArrayElement = 0;
		descriptor_write.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptor_write.descriptorCount = 1;
		descriptor_write.pImageInfo = &image_info;

		vkUpdateDescriptorSets(state.device, 1, &descriptor_write, 0, nullptr);
	}

	// Create compute descriptors
	{
		VkDescriptorSetLayoutBinding storage_sampler_binding{};

		storage_sampler_binding.binding    = 0;
		storage_sampler_binding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

		storage_sampler_binding.descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		storage_sampler_binding.descriptorCount = 1;

		VkDescriptorSetLayoutCreateInfo layout_info{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
		layout_info.bindingCount = 1;
		layout_info.pBindings    = &storage_sampler_binding;

		vkCreateDescriptorSetLayout(state.device, &layout_info, nullptr, &state.compute_descset_layout);

		VkDescriptorPoolSize pool_size{};

		pool_size.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;

		pool_size.descriptorCount = 1;

		VkDescriptorPoolCreateInfo pool_info{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};

		pool_info.poolSizeCount = 1;
		pool_info.pPoolSizes    = &pool_size;
		pool_info.maxSets       = 1;

		vkCreateDescriptorPool(state.device, &pool_info, nullptr, &state.compute_desc_pool);

		VkDescriptorSetAllocateInfo alloc_info{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
		alloc_info.descriptorPool     = state.compute_desc_pool;
		alloc_info.descriptorSetCount = 1;
		alloc_info.pSetLayouts        = &state.compute_descset_layout;

		vkAllocateDescriptorSets(state.device, &alloc_info, &state.compute_descset);

		VkDescriptorImageInfo image_info{};

		image_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

		image_info.imageView = state.raytrace_storage_image_view;
		image_info.sampler   = state.raytrace_storage_image_sampler;

		VkWriteDescriptorSet descriptor_write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};

		descriptor_write.dstSet = state.compute_descset;
		descriptor_write.dstBinding = 0;
		descriptor_write.dstArrayElement = 0;
		descriptor_write.descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		descriptor_write.descriptorCount = 1;
		descriptor_write.pImageInfo = &image_info;

		vkUpdateDescriptorSets(state.device, 1, &descriptor_write, 0, nullptr);
	}

	// Create graphics pipeline
	{
		auto vert_shader_code = ReadFile("../Assets/Compiled/Fullscreen.vert.spv");
        auto frag_shader_code = ReadFile("../Assets/Compiled/Fullscreen.frag.spv");

        VkShaderModuleCreateInfo vert_module_info{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
        vert_module_info.codeSize = vert_shader_code.size();
        vert_module_info.pCode    = reinterpret_cast<const uint32_t *>(vert_shader_code.data());

        VkShaderModule vert_shader_module;
        vkCreateShaderModule(state.device, &vert_module_info, nullptr, &vert_shader_module);

        VkShaderModuleCreateInfo frag_module_info{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
        frag_module_info.codeSize = frag_shader_code.size();
        frag_module_info.pCode    = reinterpret_cast<const uint32_t *>(frag_shader_code.data());

        VkShaderModule frag_shader_module;
        vkCreateShaderModule(state.device, &frag_module_info, nullptr, &frag_shader_module);

        VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
        vertShaderStageInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertShaderStageInfo.stage  = VK_SHADER_STAGE_VERTEX_BIT;
        vertShaderStageInfo.module = vert_shader_module;
        vertShaderStageInfo.pName  = "main";

        VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
        fragShaderStageInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragShaderStageInfo.stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragShaderStageInfo.module = frag_shader_module;
        fragShaderStageInfo.pName  = "main";

        VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

        VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexBindingDescriptionCount = 0;
        vertexInputInfo.vertexAttributeDescriptionCount = 0;

        VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        VkViewport viewport = {};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = (float) state.swapchain.extent.width;
        viewport.height = (float) state.swapchain.extent.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        VkRect2D scissor = {};
        scissor.offset = {0, 0};
        scissor.extent = state.swapchain.extent;

        VkPipelineViewportStateCreateInfo viewportState = {};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.pViewports = &viewport;
        viewportState.scissorCount = 1;
        viewportState.pScissors = &scissor;

        VkPipelineRasterizationStateCreateInfo rasterizer = {};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VK_CULL_MODE_FRONT_BIT;
        rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterizer.depthBiasEnable = VK_FALSE;

        VkPipelineMultisampleStateCreateInfo multisampling = {};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_FALSE;

        VkPipelineColorBlendStateCreateInfo colorBlending = {};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.logicOp = VK_LOGIC_OP_COPY;
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;
        colorBlending.blendConstants[0] = 0.0f;
        colorBlending.blendConstants[1] = 0.0f;
        colorBlending.blendConstants[2] = 0.0f;
        colorBlending.blendConstants[3] = 0.0f;

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
		pipelineLayoutInfo.setLayoutCount = 1;
		pipelineLayoutInfo.pSetLayouts    = &state.graphics_descset_layout;

        vkCreatePipelineLayout(state.device, &pipelineLayoutInfo, nullptr, &state.graphics_pipeline_layout);

        VkGraphicsPipelineCreateInfo pipelineInfo = {};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = shaderStages;
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.layout = state.graphics_pipeline_layout;
        pipelineInfo.renderPass = state.render_pass;
        pipelineInfo.subpass = 0;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

        vkCreateGraphicsPipelines(state.device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &state.graphics_pipeline);

        vkDestroyShaderModule(state.device, frag_shader_module, nullptr);
        vkDestroyShaderModule(state.device, vert_shader_module, nullptr);
	}

	// Create compute pipeline
	{
		const auto comp_shader_code = ReadFile("../Assets/Compiled/Tracer.comp.spv");

        VkShaderModuleCreateInfo comp_module_info{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
        comp_module_info.codeSize = comp_shader_code.size();
        comp_module_info.pCode    = reinterpret_cast<const uint32_t *>(comp_shader_code.data());

        VkShaderModule comp_shader_module;
        vkCreateShaderModule(state.device, &comp_module_info, nullptr, &comp_shader_module);

		VkPipelineShaderStageCreateInfo compute_shader_info{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};

		compute_shader_info.stage  = VK_SHADER_STAGE_COMPUTE_BIT;
		compute_shader_info.module = comp_shader_module;
		compute_shader_info.pName  = "main";

		VkPushConstantRange push_constant_range{};

		push_constant_range.offset = 0;
		push_constant_range.size   = sizeof(FrameData);

		push_constant_range.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

		VkPipelineLayoutCreateInfo compute_pipeline_layout_info{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};

		compute_pipeline_layout_info.setLayoutCount = 1;
		compute_pipeline_layout_info.pSetLayouts    = &state.compute_descset_layout;

		compute_pipeline_layout_info.pushConstantRangeCount = 1;
		compute_pipeline_layout_info.pPushConstantRanges    = &push_constant_range;

		vkCreatePipelineLayout(state.device, &compute_pipeline_layout_info, nullptr, &state.compute_pipeline_layout);

		VkComputePipelineCreateInfo compute_pipeline_info{VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};

		compute_pipeline_info.layout = state.compute_pipeline_layout;
		compute_pipeline_info.stage  = compute_shader_info;

		vkCreateComputePipelines(state.device, VK_NULL_HANDLE, 1, &compute_pipeline_info, nullptr, &state.compute_pipeline);

		vkDestroyShaderModule(state.device, comp_shader_module, nullptr);
	}

	return Error::SUCCESS;
}

GraphicsDevice::Error GraphicsDevice::Destruct()
{
 	vkDeviceWaitIdle(state.device);

	for (unsigned char i = 0; i < state.FRAMES_IN_FLIGHT; ++i)
	{
		vkDestroyFence(state.device, state.swapchain.frameFences[i], nullptr);

		vkDestroySemaphore(state.device, state.swapchain.imageAvailableSemaphores[i], nullptr);
		vkDestroySemaphore(state.device, state.swapchain.renderFinishedSemaphores[i], nullptr);
	}

	vkDestroyPipeline(state.device, state.graphics_pipeline, nullptr);
	vkDestroyPipeline(state.device, state.compute_pipeline, nullptr);

	vkDestroyPipelineLayout(state.device, state.graphics_pipeline_layout, nullptr);
	vkDestroyPipelineLayout(state.device, state.compute_pipeline_layout, nullptr);

	vkDestroyDescriptorPool(state.device, state.graphics_desc_pool, nullptr);
	vkDestroyDescriptorPool(state.device, state.compute_desc_pool, nullptr);

	vkDestroyDescriptorSetLayout(state.device, state.graphics_descset_layout, nullptr);
	vkDestroyDescriptorSetLayout(state.device, state.compute_descset_layout, nullptr);

	vkDestroySampler(state.device, state.raytrace_storage_image_sampler, nullptr);

	vkDestroyImageView(state.device, state.raytrace_storage_image_view, nullptr);

	vkDestroyImage(state.device, state.raytrace_storage_image, nullptr);

	vkFreeMemory(state.device, state.raytrace_storage_image_memory, nullptr);

	for (const auto & framebuffer : state.swapchain.framebuffers)
	{
		vkDestroyFramebuffer(state.device, framebuffer, nullptr);
	}

	vkDestroyRenderPass(state.device, state.render_pass, nullptr);

	for (const auto & imageView : state.swapchain.imageViews)
	{
		vkDestroyImageView(state.device, imageView, nullptr);
	}

	vkDestroySwapchainKHR(state.device, state.swapchain.swapchain, nullptr);

	for (const auto & command_pool : state.commandPools)
	{
		vkDestroyCommandPool(state.device, command_pool, nullptr);
	}

	vkDestroyDevice(state.device, nullptr);

	vkDestroyInstance(state.instance, nullptr);

	return Error::SUCCESS;
}

void GraphicsDevice::Draw(const FrameData & frame_data)
{
	vkWaitForFences(state.device, 1, &state.swapchain.frameFences[state.currentFrame], VK_TRUE, std::numeric_limits<uint64_t>::max());
    vkResetFences(state.device, 1, &state.swapchain.frameFences[state.currentFrame]);

	vkResetCommandPool(state.device, state.commandPools[state.currentFrame], 0);

	uint32_t image_idx;
    vkAcquireNextImageKHR(state.device, state.swapchain.swapchain, std::numeric_limits<uint64_t>::max(), state.swapchain.imageAvailableSemaphores[state.currentFrame], VK_NULL_HANDLE, &image_idx);

	const auto & command_buffer = state.commandBuffers[state.currentFrame];

	VkCommandBufferBeginInfo begin_info{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };

    begin_info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

    vkBeginCommandBuffer(command_buffer, &begin_info);

	{
		VkImageMemoryBarrier imageMemoryBarrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};

		imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;

		imageMemoryBarrier.image = state.raytrace_storage_image;

		imageMemoryBarrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
		imageMemoryBarrier.srcAccessMask    = VK_ACCESS_SHADER_WRITE_BIT;
		imageMemoryBarrier.dstAccessMask    = VK_ACCESS_SHADER_READ_BIT;

		vkCmdPipelineBarrier(
			command_buffer,
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			0,
			0, nullptr,
			0, nullptr,
			1, &imageMemoryBarrier);

		vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, state.compute_pipeline);

		vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, state.compute_pipeline_layout, 0, 1, &state.compute_descset, 0, nullptr);

		FrameData frame_data_real = frame_data;

		frame_data_real.aspect_ratio = static_cast<float>(state.swapchain.extent.width) / static_cast<float>(state.swapchain.extent.height);

		frame_data_real.seed = glfwGetTime();

		vkCmdPushConstants(command_buffer, state.compute_pipeline_layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(FrameData), &frame_data_real);

		vkCmdDispatch(command_buffer, state.RAYTRACE_RESOLUTION / 16, state.RAYTRACE_RESOLUTION / 16, 1);

		imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
		imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		imageMemoryBarrier.image = state.raytrace_storage_image;

		imageMemoryBarrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
		imageMemoryBarrier.srcAccessMask    = VK_ACCESS_SHADER_WRITE_BIT;
		imageMemoryBarrier.dstAccessMask    = VK_ACCESS_SHADER_READ_BIT;

		vkCmdPipelineBarrier(
			command_buffer,
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			0,
			0, nullptr,
			0, nullptr,
			1, &imageMemoryBarrier);

		VkRenderPassBeginInfo pass_begin_info{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};

		pass_begin_info.renderPass  = state.render_pass;
		pass_begin_info.framebuffer = state.swapchain.framebuffers[image_idx];

		pass_begin_info.renderArea.offset = {0, 0};
		pass_begin_info.renderArea.extent = state.swapchain.extent;

		VkClearValue clear_color{1.0f, 0.0f, 0.0f, 1.0f};

		pass_begin_info.clearValueCount = 1;
		pass_begin_info.pClearValues    = &clear_color;

		vkCmdBeginRenderPass(command_buffer, &pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

			vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, state.graphics_pipeline);

			vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, state.graphics_pipeline_layout, 0, 1, &state.graphics_descset, 0, nullptr);

			vkCmdDraw(command_buffer, 3, 1, 0, 0);

		vkCmdEndRenderPass(command_buffer);
	}

	vkEndCommandBuffer(command_buffer);

	VkSubmitInfo submit_info{ VK_STRUCTURE_TYPE_SUBMIT_INFO };

	VkPipelineStageFlags wait_stages[]{ VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

	submit_info.waitSemaphoreCount = 1;
	submit_info.pWaitSemaphores    = &state.swapchain.imageAvailableSemaphores[state.currentFrame];
	submit_info.pWaitDstStageMask  = wait_stages;

	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers    = &command_buffer;

	submit_info.signalSemaphoreCount = 1;
	submit_info.pSignalSemaphores    = &state.swapchain.renderFinishedSemaphores[state.currentFrame];

	vkQueueSubmit(state.graphicsQueue, 1, &submit_info, state.swapchain.frameFences[state.currentFrame]);

	VkPresentInfoKHR present_info{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};

	present_info.waitSemaphoreCount = 1;
	present_info.pWaitSemaphores    = &state.swapchain.renderFinishedSemaphores[state.currentFrame];

	present_info.swapchainCount = 1;
	present_info.pSwapchains    = &state.swapchain.swapchain;
	present_info.pImageIndices  = &image_idx;

	vkQueuePresentKHR(state.presentQueue, &present_info);

	state.currentFrame = (state.currentFrame + 1) % state.FRAMES_IN_FLIGHT;
}

void GraphicsDevice::WaitIdle()
{
	vkDeviceWaitIdle(state.device);
}
