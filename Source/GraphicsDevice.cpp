#include <GraphicsDevice.h>
#include "VulkanState.h"

#include <GLFW/glfw3.h>

#include <algorithm>
#include <cstring>
#include <iostream>
#include <vector>

struct GraphicsDevice::Framebuffer_T
{
	VkFramebuffer handle;
};

struct GraphicsDevice::Pipeline_T
{
	VkPipeline handle;
};

struct GraphicsDevice::ImageView_T
{
	VkImageView handle;

	unsigned short width;
	unsigned short height;
};

struct GraphicsDevice::RenderPass_T
{
	VkRenderPass handle;
};

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

GraphicsDevice::Error GraphicsDevice::Construct(const GraphicsDevice::CreateInfo & info)
{
	// Create instance
	{
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

		if (vkCreateCommandPool(state.device, &poolInfo, nullptr, &state.commandPool) != VK_SUCCESS)
		{
			std::cout << "[app] - err :: Failed to create command pool" << std::endl;
			return Error::UNKNOWN;
		}

		VkCommandBufferAllocateInfo allocInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
		allocInfo.commandPool = state.commandPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = 1;

		if (vkAllocateCommandBuffers(state.device, &allocInfo, &state.commandBuffer) != VK_SUCCESS)
		{
			std::cout << "[app] - err :: Failed to create command buffers" << std::endl;
			return Error::UNKNOWN;
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

			state.swapchain.imageViews[i].width  = state.swapchain.extent.width;
			state.swapchain.imageViews[i].height = state.swapchain.extent.height;

			if (vkCreateImageView(state.device, &createInfo, nullptr, &state.swapchain.imageViews[i].handle) != VK_SUCCESS)
			{
				return Error::NO_SUITABLE_SURFACE;
			}
		}

		// Create semaphores

		VkSemaphoreCreateInfo semaphoreInfo{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};

		if (vkCreateSemaphore(state.device, &semaphoreInfo, nullptr, &state.swapchain.imageAvailableSemaphore) != VK_SUCCESS
			|| vkCreateSemaphore(state.device, &semaphoreInfo, nullptr, &state.swapchain.renderFinishedSemaphore) != VK_SUCCESS)
		{
			std::cout << "[app] - err :: Failed to create swapchain semaphores" << std::endl;
			return Error::UNKNOWN;
		}

		VkFenceCreateInfo fenceInfo{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};

		if (vkCreateFence(state.device, &fenceInfo, nullptr, &state.swapchain.frameFence) != VK_SUCCESS)
		{
			std::cout << "[app] - err :: Failed to create swapchain fence" << std::endl;
		}

		vkResetFences(state.device, 1, &state.swapchain.frameFence);
	}

	return Error::SUCCESS;
}

GraphicsDevice::Error GraphicsDevice::Destruct()
{
 	vkDeviceWaitIdle(state.device);

	vkDestroyFence(state.device, state.swapchain.frameFence, nullptr);
	vkDestroySemaphore(state.device, state.swapchain.imageAvailableSemaphore, nullptr);
	vkDestroySemaphore(state.device, state.swapchain.renderFinishedSemaphore, nullptr);

	for (const auto & imageView : state.swapchain.imageViews)
	{
		vkDestroyImageView(state.device, imageView.handle, nullptr);
	}

	vkDestroySwapchainKHR(state.device, state.swapchain.swapchain, nullptr);

	vkDestroyCommandPool(state.device, state.commandPool, nullptr);

	vkDestroyDevice(state.device, nullptr);

	vkDestroyInstance(state.instance, nullptr);

	return Error::SUCCESS;
}

GraphicsDevice::RenderPass GraphicsDevice::CreateRenderPass(const RenderPassInfo & info)
{
	RenderPass pass
	{
		.data = new RenderPass_T{},
		.count = 1
	};

	std::vector<VkAttachmentDescription> colorAttachments;
	std::vector<VkSubpassDescription>    subpasses;

	// [ FAIR WORD OF WARNING ]
	//
	// Everything in this function, from this point onwards, is
	// quite easily THE HACKIEST code in this codebase.  You are
	// not expected to understand the witchcraft by which it
	// operates, yet you are not expected to maintain it, either.
	//
	// Just please don't touch this code.

	VkAttachmentReference * references [255];
	unsigned char referenceCount = 0;

	for (unsigned short i = 0; i < info.colorAttachmentCount; ++i)
	{
		VkAttachmentDescription attachment{};
		attachment.format  = info.colorAttachments[i].format == ImageFormat::SWAPCHAIN ? state.swapchain.surfaceFormat.format : imageFormatTable[static_cast<unsigned char>(info.colorAttachments[i].format)];
		attachment.samples = VK_SAMPLE_COUNT_1_BIT;
		attachment.loadOp  = loadOpTable[static_cast<unsigned char>(info.colorAttachments[i].loadOp)];
		attachment.storeOp = storeOpTable[static_cast<unsigned char>(info.colorAttachments[i].storeOp)];

		attachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachment.initialLayout  = imageLayoutTable[static_cast<unsigned char>(info.colorAttachments[i].initialLayout)];
		attachment.finalLayout    = imageLayoutTable[static_cast<unsigned char>(info.colorAttachments[i].finalLayout)];

		colorAttachments.push_back(attachment);
	}

	std::cout << "[app] - dbg :: Creating RenderPass with colorAttachmentCount=" << info.colorAttachmentCount << ", subpassCount=" << info.subpassCount << std::endl;

	for (unsigned short i = 0; i < info.subpassCount; ++i)
	{
		VkSubpassDescription subpass{};

		unsigned int startingAttachmentIdx = referenceCount;

		std::cout << "[app] - dbg :: Creating subpass with colorAttachmentCount=" << static_cast<unsigned int>(info.subpasses[i].colorReferenceCount) << std::endl;

		for (unsigned short j = 0; j < info.subpasses[i].colorReferenceCount; ++j)
		{
			auto & reference = references[referenceCount++] = new VkAttachmentReference{};
			reference->attachment = info.subpasses[i].colorReferences[j];
			reference->layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

			std::cout << "[app] - dbg :: Creating reference to color attachment " << info.subpasses[i].colorReferences[j] << " in subpass " << i << std::endl;
		}

		subpass.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = info.subpasses[i].colorReferenceCount;
		subpass.pColorAttachments    = references[startingAttachmentIdx];

		subpasses.push_back(subpass);
	}

	VkSubpassDependency dependency{};

	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;

	dependency.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	VkRenderPassCreateInfo renderPassInfo{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};

	renderPassInfo.attachmentCount = colorAttachments.size();
	renderPassInfo.pAttachments    = colorAttachments.data();

	renderPassInfo.subpassCount = subpasses.size();
	renderPassInfo.pSubpasses   = subpasses.data();

	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies   = &dependency;

	if (vkCreateRenderPass(state.device, &renderPassInfo, nullptr, &pass.data->handle) != VK_SUCCESS)
	{
		// We failed chief
		std::cout << "[app] - err :: Failed to create render pass" << std::endl;
		return {};
	}

	for (unsigned char i = 0; i < referenceCount; ++i)
	{
		delete references[i];
	}

	return pass;
}

void GraphicsDevice::DestroyRenderPass(const RenderPass & pass)
{
	vkDestroyRenderPass(state.device, pass.data->handle, nullptr);

	delete pass.data;
}

GraphicsDevice::Framebuffer GraphicsDevice::CreateFramebuffer(const RenderPass & pass, const ImageView & imageView)
{
	Framebuffer framebuffer
	{
		.data  = new Framebuffer_T [imageView.count] {},
		.count = imageView.count
	};

	for (unsigned char i = 0; i < imageView.count; ++i)
	{
		VkFramebufferCreateInfo framebufferInfo{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};

		framebufferInfo.renderPass = pass.data->handle;

		framebufferInfo.attachmentCount = 1;
		framebufferInfo.pAttachments    = &imageView.data[i].handle;

		framebufferInfo.width  = imageView.data[i].width;
		framebufferInfo.height = imageView.data[i].height;
		framebufferInfo.layers = 1;

		if (vkCreateFramebuffer(state.device, &framebufferInfo, nullptr, &framebuffer.data[i].handle) != VK_SUCCESS)
		{
			std::cout << "[app] - err :: Failed to create framebuffer" << std::endl;
			return {};
		}
	}

	return framebuffer;
}

void GraphicsDevice::DestroyFramebuffer(const Framebuffer & framebuffer)
{
	for (unsigned char i = 0; i < framebuffer.count; ++i)
	{
		vkDestroyFramebuffer(state.device, framebuffer.data[i].handle, nullptr);
	}

	delete framebuffer.data;
}

GraphicsDevice::Pipeline GraphicsDevice::CreateGraphicsPipeline(const PipelineInfo & info)
{
	Pipeline pipeline
	{
		.data = new Pipeline_T{}
	};

	VkPipelineVertexInputStateCreateInfo vertexInputInfo{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
	vertexInputInfo.vertexBindingDescriptionCount   = 0;
	vertexInputInfo.vertexAttributeDescriptionCount = 0;

	VkPipelineInputAssemblyStateCreateInfo inputAssembly{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width  = (float) state.swapchain.extent.width;
	viewport.height = (float) state.swapchain.extent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor{};
	scissor.offset = {0, 0};
	scissor.extent = state.swapchain.extent;

	VkPipelineViewportStateCreateInfo viewportState{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissor;

	VkPipelineRasterizationStateCreateInfo rasterizer{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
	rasterizer.depthBiasEnable = VK_FALSE;

	VkPipelineMultisampleStateCreateInfo multisampling{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	VkPipelineColorBlendAttachmentState colorBlendAttachment{};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;

	VkPipelineColorBlendStateCreateInfo colorBlending{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY;
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;
	colorBlending.blendConstants[0] = 0.0f;
	colorBlending.blendConstants[1] = 0.0f;
	colorBlending.blendConstants[2] = 0.0f;
	colorBlending.blendConstants[3] = 0.0f;

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
	pipelineLayoutInfo.setLayoutCount = 0;
	pipelineLayoutInfo.pushConstantRangeCount = 0;

	VkPipelineLayout pipelineLayout;

	if (vkCreatePipelineLayout(state.device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
	{
		std::cout << "[app] - err :: Failed to create graphics pipeline layout" << std::endl;
		return {};
	}

	std::vector<VkPipelineShaderStageCreateInfo> shaderStages{info.shaderStageCount};

	for (unsigned char i = 0; i < info.shaderStageCount; ++i)
	{
		VkShaderModuleCreateInfo createInfo{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
		createInfo.codeSize = info.shaderStages[i].byteCodeSize;
		createInfo.pCode    = reinterpret_cast<const unsigned int *>(info.shaderStages[i].byteCode);

		shaderStages[i].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderStages[i].stage  = shaderStageTable[static_cast<unsigned char>(info.shaderStages[i].stage)];
		shaderStages[i].pName  = info.shaderStages[i].entryPoint;

		if (vkCreateShaderModule(state.device, &createInfo, nullptr, &shaderStages[i].module) != VK_SUCCESS)
		{
			std::cout << "[app] - err :: Failed to create shader module" << std::endl;
		}
	}

	VkGraphicsPipelineCreateInfo pipelineInfo{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};

	pipelineInfo.stageCount = info.shaderStageCount;
	pipelineInfo.pStages    = shaderStages.data();

	pipelineInfo.pVertexInputState   = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState      = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState   = &multisampling;
	pipelineInfo.pColorBlendState    = &colorBlending;

	pipelineInfo.layout     = pipelineLayout;
	pipelineInfo.renderPass = info.pass.data->handle;
	pipelineInfo.subpass    = 0;

	if (vkCreateGraphicsPipelines(state.device, nullptr, 1, &pipelineInfo, nullptr, &pipeline.data->handle))
	{
		std::cout << "[app] - err :: Failed to create graphics pipeline object" << std::endl;
		return {};
	}

	for (const auto & shaderStage : shaderStages)
	{
		vkDestroyShaderModule(state.device, shaderStage.module, nullptr);
	}

	vkDestroyPipelineLayout(state.device, pipelineLayout, nullptr);

	return pipeline;
}

void GraphicsDevice::DestroyGraphicsPipeline(const Pipeline & pipeline)
{
	vkDestroyPipeline(state.device, pipeline.data->handle, nullptr);

	delete pipeline.data;
}

GraphicsDevice::ImageView GraphicsDevice::GetSwapchainImageViews()
{
	return
	{
		.data  = state.swapchain.imageViews.data(),
		.count = state.swapchain.imageViews.size()
	};
}

void GraphicsDevice::DrawFrame(const RenderPass & pass, const Pipeline & pipeline, const Framebuffer & framebuffer)
{
	unsigned int imageIndex;
    vkAcquireNextImageKHR(state.device, state.swapchain.swapchain, std::numeric_limits<uint64_t>::max(), state.swapchain.imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

	VkCommandBufferBeginInfo beginInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    if (vkBeginCommandBuffer(state.commandBuffer, &beginInfo) != VK_SUCCESS)
	{
        std::cout << "[app] - err :: Failed to begin command buffer recording" << std::endl;
    }

	VkRenderPassBeginInfo renderPassInfo{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};

	renderPassInfo.renderPass  = pass.data->handle;
	renderPassInfo.framebuffer = framebuffer.data[imageIndex].handle;

	renderPassInfo.renderArea.offset = {0, 0};
	renderPassInfo.renderArea.extent = state.swapchain.extent;

	VkClearValue clearColor = {0.0f, 0.0f, 0.0f, 1.0f};

	renderPassInfo.clearValueCount = 1;
	renderPassInfo.pClearValues    = &clearColor;

	vkCmdBeginRenderPass(state.commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindPipeline(state.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.data->handle);

	vkCmdDraw(state.commandBuffer, 3, 1, 0, 0);

	vkCmdEndRenderPass(state.commandBuffer);

	vkEndCommandBuffer(state.commandBuffer);

	VkSubmitInfo submitInfo{VK_STRUCTURE_TYPE_SUBMIT_INFO};

	VkSemaphore waitSemaphores[] {state.swapchain.imageAvailableSemaphore};
	VkPipelineStageFlags waitStages[] {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores    = waitSemaphores;
	submitInfo.pWaitDstStageMask  = waitStages;

	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &state.commandBuffer;

	VkSemaphore signalSemaphores[]{state.swapchain.renderFinishedSemaphore};
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores    = signalSemaphores;

	if (vkQueueSubmit(state.graphicsQueue, 1, &submitInfo, state.swapchain.frameFence) != VK_SUCCESS)
	{
		std::cout << "[app] - err :: Failed to submit to graphics queue" << std::endl;
	}

	VkPresentInfoKHR presentInfo{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};

	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores    = signalSemaphores;

	VkSwapchainKHR swapChains[] {state.swapchain.swapchain};
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains    = swapChains;
	presentInfo.pImageIndices  = &imageIndex;

	vkQueuePresentKHR(state.presentQueue, &presentInfo);

	vkWaitForFences(state.device, 1, &state.swapchain.frameFence, VK_TRUE, std::numeric_limits<uint64_t>::max());
    vkResetFences(state.device, 1, &state.swapchain.frameFence);

	vkResetCommandPool(state.device, state.commandPool, 0);
}
