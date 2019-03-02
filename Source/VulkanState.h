#pragma once

#define VK_USE_PLATFORM_XCB_KHR
#include <vulkan/vulkan.h>

#include <glm/glm.hpp>

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

	std::vector<VkSemaphore> imageAvailableSemaphores;
	std::vector<VkSemaphore> renderFinishedSemaphores;

	std::vector<VkFence> frameFences;

	/// @brief Swapchain images
	std::vector<VkImage> images;

	/// @brief Image views into swapchain images
	std::vector<VkImageView> imageViews;

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

	VkRenderPass render_pass;

	VkPipeline graphics_pipeline;
	VkPipeline compute_pipeline;

	VkPipelineLayout graphics_pipeline_layout;
	VkPipelineLayout compute_pipeline_layout;

	VkQueue graphicsQueue;
	VkQueue presentQueue;

	int graphicsQueueIndex;
	int presentQueueIndex;

	VkDescriptorSetLayout graphics_descset_layout;
	VkDescriptorSetLayout compute_descset_layout;

	VkDescriptorSet  graphics_descset;
	VkDescriptorPool graphics_desc_pool;

	VkDescriptorSet  compute_descset;
	VkDescriptorPool compute_desc_pool;

	VkImage     raytrace_storage_image;
	VkImageView raytrace_storage_image_view;

	VkSampler raytrace_storage_image_sampler;

	VkDeviceMemory raytrace_storage_image_memory;

	/// @note Single threaded
	std::vector<VkCommandPool> commandPools;

	std::vector<VkCommandBuffer> commandBuffers;

	// IMMUTABLE STATE //

	unsigned char FRAMES_IN_FLIGHT = 2;

	unsigned short RAYTRACE_RESOLUTION;

	// MUTABLE STATE //

	unsigned char currentFrame;
};
