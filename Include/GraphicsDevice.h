/**
 * @todo Framebuffers.  We will need a method of creating N framebuffers at once, i.e. per chain image or per frame
 */

#pragma once

struct GLFWwindow;

enum class ImageFormat : unsigned char
{
	SWAPCHAIN
};

enum class ImageLayout : unsigned char
{
	UNDEFINED,
	COLOR_OPTIMAL,
	PRESENT
};

enum class LoadOp : unsigned char
{
	CLEAR
};

enum class StoreOp : unsigned char
{
	STORE
};

enum class ShaderStage : unsigned char
{
	VERTEX,
	FRAGMENT
};

struct AttachmentInfo final
{
	ImageFormat format;

	ImageLayout initialLayout;
	ImageLayout finalLayout;

	LoadOp  loadOp;
	StoreOp storeOp;
};

struct SubPassInfo final
{
	const unsigned short colorReferences [8];

	unsigned char colorReferenceCount;
};

struct RenderPassInfo final
{
	const AttachmentInfo * const colorAttachments;

	unsigned short colorAttachmentCount;

	const SubPassInfo * const subpasses;

	unsigned short subpassCount;
};

/**
 * @brief Platform-agnostic, explicit API for interfacing with and controlling system GPUs
 *
 * @note Single GPU setups supported.  Multiple GPUs will be ignored, with the most appropriate GPU chosen
 */
struct GraphicsDevice final
{
	/**
	 * @brief Describes the error state which a method may exhibit.
	 */
	enum class Error : signed char
	{
		SUCCESS,             //< Operation finished successfully, or no fatal errors occured
		NO_SUITABLE_GPU,     //< Graphics device was unable to find a compatible GPU
		NO_SUITABLE_SURFACE, //< Graphics device was unable to create a suitable surface
		UNKNOWN              //< Operation exhibited an error which cannot be handled by the calling code
	};

	/**
	 * @brief Information required for graphics device construction
	 */
	struct CreateInfo final
	{
		GLFWwindow * window; //< Handle to OS window object (GLFW)

		unsigned char swapchainSize; //< Recommended number of images in swapchain

		bool debug; //< Toggles debugging features during graphics device construction
	};

	struct Framebuffer_T;
	struct Pipeline_T;
	struct ImageView_T;
	struct RenderPass_T;

	struct Framebuffer
	{
		Framebuffer_T * data;

		unsigned char count;
	};

	struct Pipeline
	{
		Pipeline_T * data;

		unsigned char count;
	};

	struct ImageView
	{
		ImageView_T * data;

		unsigned char count;
	};

	struct RenderPass
	{
		RenderPass_T * data;

		unsigned char count;
	};

	struct PipelineInfo final
	{
		struct ShaderStageInfo final
		{
			const char * byteCode;
			const char * entryPoint;

			unsigned long long byteCodeSize;

			ShaderStage stage;
		};

		RenderPass pass;

		ShaderStageInfo * shaderStages;

		unsigned int shaderStageCount;
	};

	/**
	 * @brief Initializes the graphics device into a stable, usable state
	 *
	 * @param   info   Information required to construct the graphics device
	 * @return  Error  Error state of device construction
	 *
	 * @see GraphicsDevice::Error
	 * @see GraphicsDevice::CreateInfo
	 */
	Error Construct(const CreateInfo & info);

	/**
	 * @brief Unallocates graphics resources and lowers graphics device into a stable, unusable state
	 *
	 * @return Error  Error state of device destruction
	 *
	 * @see GraphicsDevice::Error
	 */
	Error Destruct();

	/**
	 * @brief Create a Render Pass object
	 *
	 * @param   info        Information used to construct render pass
	 * @return  RenderPass  Opaque handle to Render Pass object
	 *
	 * @see RenderPassInfo
	 * @see GraphicsDevice::RenderPass
	 */
	RenderPass CreateRenderPass(const RenderPassInfo & info);

	void DestroyRenderPass(const RenderPass & pass);

	Framebuffer CreateFramebuffer(const RenderPass & pass, const ImageView & imageView);

	void DestroyFramebuffer(const Framebuffer & framebuffer);

	Pipeline CreateGraphicsPipeline(const PipelineInfo & info);

	void DestroyGraphicsPipeline(const Pipeline & pipeline);

	ImageView GetSwapchainImageViews();

	void DrawFrame(const RenderPass & pass, const Pipeline & pipeline, const Framebuffer & framebuffer);
};
