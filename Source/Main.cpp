// #include <VulkanApp.h>
#include <GraphicsDevice.h>

#include <GLFW/glfw3.h>

#include <fstream>
#include <iostream>
#include <vector>

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

int main()
{
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	const auto window = glfwCreateWindow(1024, 768, "Graphics Device", nullptr, nullptr);

	GraphicsDevice device;

	const GraphicsDevice::CreateInfo info
	{
		.window = window,
		.swapchainSize = 3,
		.debug = true
	};

	if (const auto res = device.Construct(info); res != GraphicsDevice::Error::SUCCESS)
	{
		std::cout << "[app] - err :: Graphics device creation failed :: " << static_cast<unsigned int>(res) << std::endl;
	}

	GraphicsDevice::RenderPass forwardPass;

	GraphicsDevice::Framebuffer backbuffer;

	{
		AttachmentInfo colorAttachments[]
		{
			// Backbuffer attachment
			{
				.format = ImageFormat::SWAPCHAIN,

				.initialLayout = ImageLayout::UNDEFINED,
				.finalLayout   = ImageLayout::PRESENT,

				.loadOp  = LoadOp::CLEAR,
				.storeOp = StoreOp::STORE
			}
		};

		SubPassInfo subpass
		{
			.colorReferences = {0},
			.colorReferenceCount = 1
		};

		forwardPass = device.CreateRenderPass(
		{
			.colorAttachments     = colorAttachments,
			.colorAttachmentCount = sizeof(colorAttachments) / sizeof(colorAttachments[0]),

			.subpasses    = &subpass,
			.subpassCount = 1
		});

		GraphicsDevice::ImageView backbufferView = device.GetSwapchainImageViews();

		backbuffer = device.CreateFramebuffer(forwardPass, backbufferView);
	}

	GraphicsDevice::Pipeline pipeline;

	{
		const auto vertShader = ReadFile("../Assets/vert.spv");
		const auto fragShader = ReadFile("../Assets/frag.spv");

		GraphicsDevice::PipelineInfo::ShaderStageInfo shaderStages[]
		{
			{
				.byteCode   = vertShader.data(),
				.entryPoint = "main",

				.byteCodeSize = vertShader.size(),

				.stage = ShaderStage::VERTEX
			},
			{
				.byteCode   = fragShader.data(),
				.entryPoint = "main",

				.byteCodeSize = fragShader.size(),

				.stage = ShaderStage::FRAGMENT
			}
		};

		pipeline = device.CreateGraphicsPipeline(
		{
			.pass = forwardPass,
			.shaderStages = shaderStages,
			.shaderStageCount = sizeof(shaderStages) / sizeof(shaderStages[0])
		});
	}

	double previousTime = glfwGetTime();
	int frameCount = 0;

	while (glfwWindowShouldClose(window) == false)
	{
		double currentTime = glfwGetTime();
		++frameCount;

		if (currentTime - previousTime >= 1.0)
		{
			std::cout << frameCount << " FPS" << std::endl;

			frameCount = 0;
			previousTime = currentTime;
		}

		glfwPollEvents();

		device.DrawFrame(forwardPass, pipeline, backbuffer);

		glfwSwapBuffers(window);
	}

	device.DestroyFramebuffer(backbuffer);

	device.DestroyGraphicsPipeline(pipeline);

	device.DestroyRenderPass(forwardPass);

	device.Destruct();

	glfwDestroyWindow(window);
	glfwTerminate();



	// VulkanApp app{1024, 768};

	// try
	// {
	// 	app.Run();
	// }
	// catch (const std::runtime_error & err)
	// {
	// 	std::cerr << err.what() << std::endl;

	// 	int c;

	// 	std::cin >> c;

	// 	return 1;
	// }

	// return 0;
}
