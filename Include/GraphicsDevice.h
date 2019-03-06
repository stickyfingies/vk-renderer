/**
 * @todo Framebuffers.  We will need a method of creating N framebuffers at once, i.e. per chain image or per frame
 */

#pragma once

#include <Camera.h>

#include <glm/glm.hpp>

struct GLFWwindow;

struct FrameData
{
	alignas(4) float aspect_ratio;

	alignas(4) float seed;

	alignas(16) glm::vec3 light_pos;

	alignas(16) CameraData camera;
};

/**
 * @brief Platform-agnostic, explicit API for interfacing with and controlling system GPUs
 *
 * @note Single GPU setups supported.  Multiple GPUs will be ignored, with the most appropriate GPU chosen
 *
 * @note Swapchain is first class citizen of graphics device
 * @note Pipelines and render passes.  Oh, pipelines and render passes.
 * @note Command submission!
 *
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
		/// @brief Handle to OS window object (GLFW)
		GLFWwindow * window;

		/// @brief Desired number of backbuffer images in swapchain
		unsigned char swapchainSize;

		/// @brief Number of frames which may be 'in-flight' (processed) at once
		unsigned char framesInFlight;

		unsigned short raytrace_resolution;

		/// @brief Toggles debugging features during graphics device construction
		bool debug;
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

	void Draw(const FrameData & frame_data);

	void WaitIdle();
};
