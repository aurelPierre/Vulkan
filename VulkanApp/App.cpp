#include "App.h"

#include <vector>
#include <map>
#include <set>

#include "Logging.h"

namespace core
{
	void App::run()
	{
		initWindow();
		initVulkan();
		loop();
		clean();
	}

	void App::initWindow()
	{
		glfwInit();
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
		_window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
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
		while (!glfwWindowShouldClose(_window)) {
			glfwPollEvents();
			_renderer.draw(_device);
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
