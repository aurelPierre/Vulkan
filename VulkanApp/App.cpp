#include "App.h"

#include <vector>
#include <map>
#include <set>

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
		_swapChain.init(_device, _surface);
		_renderPass.init(_device, _swapChain);
		_swapChain.initBuffers(_device, _renderPass.getRenderPass(), _renderPass.getPipeline());
		createSemaphores();
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

	void App::createSemaphores()
	{
		VkSemaphoreCreateInfo semaphoreInfo = {};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkResult result = vkCreateSemaphore(_device.getDevice(), &semaphoreInfo, nullptr, &_imageAvailableSemaphore);
		if (result != VK_SUCCESS)
			THROW("failed to create semaphore with error: " + result)

		result = vkCreateSemaphore(_device.getDevice(), &semaphoreInfo, nullptr, &_renderFinishedSemaphore);
		if (result != VK_SUCCESS)
			THROW("failed to create semaphore with error: " + result)
	}

	void App::loop()
	{
		while (!glfwWindowShouldClose(_window)) {
			glfwPollEvents();
			drawFrame();
		}

		vkDeviceWaitIdle(_device.getDevice());
	}

	void App::drawFrame()
	{
		vkQueueWaitIdle(_device.getPresentQueue());

		uint32_t imageIndex;
		vkAcquireNextImageKHR(_device.getDevice(), _swapChain.getSwapchainKHR(), std::numeric_limits<uint64_t>::max(), _imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		VkSemaphore waitSemaphores[] = { _imageAvailableSemaphore };
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &_swapChain.getCommandBuffer(imageIndex);

		VkSemaphore signalSemaphores[] = { _renderFinishedSemaphore };
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;

		VkResult result = vkQueueSubmit(_device.getGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE);
		if (result != VK_SUCCESS)
			THROW("failed to submit draw command buffer with error: " + result)

		VkPresentInfoKHR presentInfo = {};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = signalSemaphores;

		VkSwapchainKHR swapChains[] = { _swapChain.getSwapchainKHR() };
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapChains;
		presentInfo.pImageIndices = &imageIndex;
		presentInfo.pResults = nullptr;

		vkQueuePresentKHR(_device.getPresentQueue(), &presentInfo);
	}

	void App::clean()
	{
		vkDestroySemaphore(_device.getDevice(), _renderFinishedSemaphore, nullptr);
		vkDestroySemaphore(_device.getDevice(), _imageAvailableSemaphore, nullptr);

		_swapChain.cleanBuffers(_device);
		_renderPass.clean(_device);
		_swapChain.clean(_device);
		_device.clean();

		vkDestroySurfaceKHR(_vkInstance, _surface, nullptr);
		vkDestroyInstance(_vkInstance, nullptr);
		glfwDestroyWindow(_window);
		glfwTerminate();
	}
}
