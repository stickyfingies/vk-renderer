#pragma once

#include <GraphicsDevice.h>

#define VK_USE_PLATFORM_XCB_KHR
#include <vulkan/vulkan.h>

#include <vector>

struct Swapchain
{
	/// @brief Handle to Vulkan swapchain
	VkSwapchainKHR swapchain;

	/// @brief Surface pixel format
	VkSurfaceFormatKHR surfaceFormat;

	/// @brief Method of presenting images to display
	VkPresentModeKHR presentMode;

	/// @brief Swapchain dimentions
	VkExtent2D extent;

	VkSemaphore imageAvailableSemaphore;
	VkSemaphore renderFinishedSemaphore;

	VkFence frameFence;

	/// @brief Swapchain images
	std::vector<VkImage> images;

	/// @brief Image views into swapchain images
	std::vector<GraphicsDevice::ImageView_T> imageViews;

	std::vector<VkFramebuffer> framebuffers;
};

/**
 * @brief Full state of vulkan backend
 *
 * @note Single threaded
 * @note Single frame in flight
 * @note Single display
 * @note Single GPU
 */
struct VulkanState
{
	VkInstance instance;

	/// @note Single display
	VkSurfaceKHR surface;

	/// @note Single display
	Swapchain swapchain;

	/// @brief Single GPU
	VkPhysicalDevice physicalDevice;

	/// @note Single GPU
	VkDevice device;

	VkQueue graphicsQueue;
	VkQueue presentQueue;

	int graphicsQueueIndex;
	int presentQueueIndex;

	VkDescriptorPool descriptorPool;

	/// @note Single threaded
	VkCommandPool commandPool;

	/// @note Single frame in flight
	VkCommandBuffer commandBuffer;
};
