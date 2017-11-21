#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

#include <stdexcept>
#include <vector>
#include <array>

#include "NonCopyable.h"
#include "Device.h"
#include "Renderer.h"

typedef unsigned int uint;

namespace core
{
	class App : public util::NonCopyable
	{
	public:
		const uint WIDTH = 800;
		const uint HEIGHT = 600;

		App() = default;
		~App() = default;

		void getWindowSize(int& window, int& height);
		void resize();

		void run();

	private:
		static void onWindowResized(GLFWwindow*, int width, int height);

		VkInstance	_vkInstance;
		GLFWwindow*	_window;
		VkSurfaceKHR _surface;

		Device _device;
		Renderer _renderer;

		void initWindow();
		void initVulkan();
		void createInstance();
		void createSurface();

		void loop();
		void clean();
	};
}
