#include <GraphicsDevice.h>
#include <Camera.h>

#include <GLFW/glfw3.h>

#include <chrono>
#include <iostream>
#include <thread>
#include <vector>

namespace
{
	FrameData frame_data;

	Camera camera;

	float last_mouse_x;
	float last_mouse_y;

	bool locked_to_camera = false;
}

static void poll_keyboard(GLFWwindow * window, float delta_time)
{
	const float sensitivity = 111.0f;

	if (glfwGetKey(window, GLFW_KEY_W))
	{
		camera.move_forward(sensitivity * delta_time);
	}
	else if (glfwGetKey(window, GLFW_KEY_S))
	{
		camera.move_backward(sensitivity * delta_time);
	}
	if (glfwGetKey(window, GLFW_KEY_A))
	{
		camera.move_left(sensitivity * delta_time);
	}
	else if (glfwGetKey(window, GLFW_KEY_D))
	{
		camera.move_right(sensitivity * delta_time);
	}
	if (glfwGetKey(window, GLFW_KEY_R))
	{
		camera.move_up(sensitivity * delta_time);
	}
	else if (glfwGetKey(window, GLFW_KEY_F))
	{
		camera.move_down(sensitivity * delta_time);
	}
}

static void key_callback(GLFWwindow * window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_E && action == GLFW_PRESS)
	{
		locked_to_camera = !locked_to_camera;
	}
}

static void mouse_callback(GLFWwindow * window, double pos_x, double pos_y)
{
	float offset_x = pos_x - last_mouse_x;
	float offset_y = last_mouse_y - pos_y;

	last_mouse_x = pos_x;
	last_mouse_y = pos_y;

	const float sensitivity = 0.21f;

	offset_x *= sensitivity;
	offset_y *= sensitivity;

	camera.aux.yaw   -= offset_x;
	camera.aux.pitch += offset_y;

	camera.update();
}

int main()
{
	// Create window

	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	const auto window = glfwCreateWindow(1024, 768, "Graphics Device", nullptr, nullptr);

	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	glfwSetKeyCallback(window, key_callback);
	glfwSetCursorPosCallback(window, mouse_callback);

	// Create graphics device

	GraphicsDevice device;

	{
		// Describe graphics device

		const GraphicsDevice::CreateInfo info
		{
			.window = window,

			.swapchainSize  = 3,
			.framesInFlight = 2,

			.raytrace_resolution = 1024,

			.debug = true
		};

		// Construct graphics device

		if (const auto res = device.Construct(info); res != GraphicsDevice::Error::SUCCESS)
		{
			std::cout << "[app] - err :: Graphics device creation failed :: " << static_cast<unsigned int>(res) << std::endl;
		}
	}

	// Set up main loop

	double previous_time{glfwGetTime()};

	unsigned int frame_count{0};

	float delta_time = 0.0f;
	float last_frame = 0.0f;

	frame_data.light_pos  = glm::vec3(0.0f, 64.0f, 0.0f);

	// Main game loop

	while (glfwWindowShouldClose(window) == false)
	{
		// FPS calculations

		const double current_time = glfwGetTime();
		++frame_count;

		// Print FPS

		if (current_time - previous_time >= 1.0)
		{
			std::cout << frame_count << " FPS" << std::endl;

			frame_count   = 0;
			previous_time = current_time;
		}

		delta_time = current_time - last_frame;
		last_frame = current_time;

		// Grab user input

		glfwPollEvents();

		poll_keyboard(window, delta_time);

		// Draw

		frame_data.camera = camera.data;

		if (locked_to_camera) frame_data.light_pos = camera.data.pos + glm::vec3(0.0, -1.0, 0.0);

		device.Draw(frame_data);

		// Swap backbuffer

		glfwSwapBuffers(window);

		// NOTE
		// If you're on a higher-end video card like me, you
		// NEED TO UNCOMMENT THE FOLLOWING LINE or your GPU will
		// SCREAM and CRY (quite literally.  It is loud.)  And it
		// will probably also overheat.
		//
		// Point being, please uncomment this next line unless
		// performance depends on it.

		// std::this_thread::sleep_for(std::chrono::milliseconds(12));
	}

	// Uninitialize

	device.WaitIdle();

	device.Destruct();

	glfwDestroyWindow(window);
	glfwTerminate();
}
