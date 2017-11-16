#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

#include <stdexcept>
#include <vector>
#include <array>

#include "NonCopyable.h"
#include "Device.h"
#include "SwapChain.h"
#include "RenderPass.h"

#ifndef _DEBUG
	#define LOGGING_DISABLE
#endif

#include "Logging.h"

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

		void run();

	private:
		VkInstance	_vkInstance;
		GLFWwindow*	_window;
		VkSurfaceKHR _surface;

		Device _device;
		SwapChain _swapChain;
		RenderPass _renderPass;
		
		VkSemaphore _imageAvailableSemaphore;
		VkSemaphore _renderFinishedSemaphore;

		void initWindow();
		void initVulkan();
		void createInstance();
		void createSurface();

		void createSemaphores();

		void loop();
		void drawFrame();
		void clean();
	};
}
