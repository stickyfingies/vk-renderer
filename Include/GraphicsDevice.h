#pragma once

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

#include <array>
#include <optional>
#include <vector>

struct GLFWwindow;

struct UniformBufferObject
{
	glm::mat4 model;
	glm::mat4 view;
	glm::mat4 proj;
};

struct QueueFamilyIndices
{
	std::optional<uint32_t> graphicsFamily;
	std::optional<uint32_t> presentFamily;

	bool Complete() const
	{
		return graphicsFamily.has_value() && presentFamily.has_value();
	}
};

struct SwapchainSupportDetails
{
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};

struct Vertex
{
	glm::vec2 pos;
	glm::vec3 color;

	static VkVertexInputBindingDescription GetBindingDescription()
	{
		VkVertexInputBindingDescription bindingDescription{};
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(Vertex);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return bindingDescription;
	}

	static std::array<VkVertexInputAttributeDescription, 2> GetAttributeDescriptions()
	{
		std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};

		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(Vertex, pos);

		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(Vertex, color);

		return attributeDescriptions;
	}
};

class GraphicsDevice
{
	GLFWwindow * window;

	//

	VkInstance instance;
	VkDebugUtilsMessengerEXT callback;
	VkSurfaceKHR surface;

	VkPhysicalDevice physicalDevice;
	VkDevice device;

	VkQueue graphicsQueue;
	VkQueue presentQueue;

	VkSwapchainKHR swapChain;
	VkFormat swapChainImageFormat;
	VkExtent2D swapChainExtent;

	VkRenderPass renderPass;
	VkPipelineLayout pipelineLayout;
	VkPipeline graphicsPipeline;

	VkDescriptorPool descriptorPool;
	VkDescriptorSetLayout descriptorSetLayout;

	VkCommandPool commandPool;

	VkBuffer vertexBuffer;
	VkDeviceMemory vertexBufferMemory;
	VkBuffer indexBuffer;
	VkDeviceMemory indexBufferMemory;

	//

	std::vector<VkCommandBuffer> commandBuffers;
	std::vector<VkImage> swapChainImages;
	std::vector<VkImageView> swapChainImageViews;
	std::vector<VkFramebuffer> swapChainFramebuffers;
	std::vector<VkSemaphore> imageAvailableSemaphores;
	std::vector<VkSemaphore> renderFinishedSemaphores;
	std::vector<VkFence> inFlightFences;
	std::vector<VkBuffer> uniformBuffers;
	std::vector<VkDeviceMemory> uniformBuffersMemory;
	std::vector<VkDescriptorSet> descriptorSets;

	//

	size_t currentFrame;

	unsigned int width;

	unsigned int height;

	bool framebufferResized;

	//

	static std::vector<char> ReadFile(const char * fileName);

	//

	bool CheckDeviceExtensionSupport(VkPhysicalDevice device);

	bool CheckValidationLayerSupport();

	VkSurfaceFormatKHR ChooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);

	VkPresentModeKHR ChooseSurfacePresentMode(const std::vector<VkPresentModeKHR> availablePresentModes);

	VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

	void Cleanup();

	void CleanupSwapchain();

	void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

	void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer & buffer, VkDeviceMemory & bufferMemory);

	void CreateCommandBuffers();

	void CreateCommandPool();

	void CreateDescriptorPool();

	void CreateDescriptorSetLayout();

	void CreateDescriptorSets();

	void CreateFramebuffers();

	void CreateGraphicsPipeline();

	void CreateImageViews();

	void CreateIndexBuffer(const void * data, unsigned int byteSize);

	void CreateInstance();

	void CreateLogicalDevice();

	void CreateRenderPass();

	VkShaderModule CreateShaderModule(const std::vector<char> & code);

	void CreateSurface();

	void CreateSwapchain();

	void CreateSyncObjects();

	void CreateUniformBuffers();

	void CreateVertexBuffer(const void * data, unsigned int byteSize);

	bool DeviceSuitable(VkPhysicalDevice device);

	void DrawFrame();

	uint64_t FindMemoryType(unsigned int typeFilter, VkMemoryPropertyFlags properties);

	QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device);

	std::vector<const char *> GetRequiredExtensions();

	void InitVulkan();

	void InitWindow();

	void MainLoop();

	void PickPhysicalDevice();

	SwapchainSupportDetails QuerySwapchainSupport(VkPhysicalDevice device);

	void RecreateSwapchain();

	void SetupDebugCallback();

	void UpdateUniformBuffer(size_t index);

public:

	GraphicsDevice(unsigned int width, unsigned int height);

	//

	void Run();

	//

	inline bool FramebufferResized() const
	{
		return framebufferResized;
	}

	inline void FramebufferResized(bool value)
	{
		framebufferResized = value;
	}
};