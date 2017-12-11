#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

#include <stdexcept>
#include <vector>
#include <array>

#include "Singleton.h"
#include "Device.h"
#include "Renderer.h"
#include "ResourceManager.h"

typedef unsigned int uint;

namespace core
{
	class App : public util::Singleton<App>
	{
	public:
		const uint WIDTH = 800;
		const uint HEIGHT = 600;

		App() = default;
		~App();

		void getWindowSize(int& window, int& height);
		void resize();

		void run();

		ResourceManager& getResourceManager() { return _resourceManager; }
		Renderer& getRenderer() { return _renderer; }
		Device& getDevice() { return _device; }

	private:
		static void onWindowResized(GLFWwindow*, int width, int height);

		VkInstance	_vkInstance;
		GLFWwindow*	_window;
		VkSurfaceKHR _surface;

		ResourceManager _resourceManager;
		Renderer _renderer;
		Device _device;

		void initWindow();
		void initVulkan();
		void createInstance();
		void createSurface();

		void loop();
		/*void clean();*/
	};
}
