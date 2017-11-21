#include "App.h"

#include <vector>
#include <map>
#include <set>
#include <chrono>

#include "Logging.h"

namespace core
{
	void App::getWindowSize(int& width, int& height)
	{
		glfwGetWindowSize(_window, &width, &height);
	}

	void App::run()
	{
		initWindow();
		initVulkan();
		loop();
		clean();
	}

	void App::onWindowResized(GLFWwindow* window, int width, int height)
	{
		if (width == 0 || height == 0)
			return;

		App* app = reinterpret_cast<App*>(glfwGetWindowUserPointer(window));
		app->resize();
	}

	void App::resize()
	{
		_renderer.recreate(_device, _surface);
	}

	void App::initWindow()
	{
		glfwInit();
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
		_window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);

		glfwSetWindowUserPointer(_window, this);
		glfwSetWindowSizeCallback(_window, App::onWindowResized);
	}

	void App::initVulkan()
	{
		createInstance();
		createSurface();
		_device.init(_vkInstance, _surface);
		_renderer.init(_device, _surface);
	}

	void App::createInstance()
	{
		VkApplicationInfo appInfo = {};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "Hello Triangle";
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = "Custom";
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_0;

		VkInstanceCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;

		uint glfwExtensionCount;
		const char** glfwExtensions;
		glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

		createInfo.enabledExtensionCount = glfwExtensionCount;
		createInfo.ppEnabledExtensionNames = glfwExtensions;

		uint extensionCount;
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
		std::vector<VkExtensionProperties> extensions(extensionCount);
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

		LOG(LogInfo, "available instance extensions:")
		for (const auto& extension : extensions)
			LOG(LogInfo, util::Log::tab << extension.extensionName)

		auto isAvailable = [&extensions](const std::string extension) -> bool {
			for (const auto& ext : extensions)
				if (extension == ext.extensionName)
					return true;
			return false;
		};

		LOG(LogInfo, "required instance extensions not supported:")
		for (uint i = 0; i < glfwExtensionCount; ++i)
		{
			if (!isAvailable(glfwExtensions[i]))
				LOG(LogError, util::Log::tab << glfwExtensions[i])
		}

		VkResult result = vkCreateInstance(&createInfo, nullptr, &_vkInstance);
		if (result != VK_SUCCESS)
			THROW("failed to create vkInstance with error: " + result)
	}

	void App::createSurface()
	{
		VkResult result = glfwCreateWindowSurface(_vkInstance, _window, nullptr, &_surface);
		if(result != VK_SUCCESS)
			THROW("failed to create window surface with error: " + result)
	}

	void App::loop()
	{
		uint fps = 0;
		float second = 0.f;
		float deltaTime;

		auto start = std::chrono::high_resolution_clock::now();
		auto end = std::chrono::high_resolution_clock::now();

		while (!glfwWindowShouldClose(_window)) {
			deltaTime = std::chrono::duration<float>(end - start).count();
			second += deltaTime;
			++fps;

			if (second > 1.f)
			{
				LOG(LogVoid, "FPS: " + std::to_string(fps));
				second = 0.f;
				fps = 0;
			}

			start = std::chrono::high_resolution_clock::now();
			
			glfwPollEvents();
			_renderer.update(_device, deltaTime);
			_renderer.draw(_device);

			end = std::chrono::high_resolution_clock::now();
		}

		vkDeviceWaitIdle(_device.getDevice());
	}

	void App::clean()
	{
		_renderer.clean(_device);
		_device.clean();

		vkDestroySurfaceKHR(_vkInstance, _surface, nullptr);
		vkDestroyInstance(_vkInstance, nullptr);
		glfwDestroyWindow(_window);
		glfwTerminate();
	}
}
