#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

#include <stdexcept>
#include <vector>
#include <array>

#include "NonCopyable.h"

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

		const std::array<const char*, 1> deviceExtensions = {
			VK_KHR_SWAPCHAIN_EXTENSION_NAME
		};

		App() = default;
		~App() = default;

		void run();

	private:
		GLFWwindow*			_window;
		VkInstance			_vkInstance;

		VkPhysicalDevice	_physicalDevice;
		VkDevice			_device;

		VkSurfaceKHR _surface;

		VkQueue _graphicsQueue;
		VkQueue _presentQueue;

		VkSwapchainKHR _swapChain;
		VkFormat _swapChainImageFormat;
		VkExtent2D _swapChainExtent;

		std::vector<VkImage> _swapChainImages;
		std::vector<VkImageView> _swapChainImageViews;

		VkRenderPass _renderPass;
		VkPipelineLayout _pipelineLayout;
		VkPipeline _graphicsPipeline;

		std::vector<VkFramebuffer> _swapChainFramebuffers;

		VkCommandPool _commandPool;
		std::vector<VkCommandBuffer> _commandBuffers;

		VkSemaphore _imageAvailableSemaphore;
		VkSemaphore _renderFinishedSemaphore;

		void initWindow();
		void initVulkan();
		void createInstance();
		void createSurface();
		void pickPhysicalDevice();
		void createLogicalDevice();
		void createSwapChain();
		void createImageViews();
		void createRenderPass();
		void createGraphicsPipeline();
		void createFramebuffers();
		void createCommandPool();
		void createCommandBuffers();
		void createSemaphores();

		VkShaderModule createShaderModule(const std::vector<char>& code);

		struct QueueFamilyIndices {
			int graphicsFamily = -1;
			int presentFamily = -1;

			bool isComplete() {
				return graphicsFamily >= 0 && presentFamily >= 0;
			}
		};

		bool isDeviceSuitable(VkPhysicalDevice);
		QueueFamilyIndices findQueueFamilies(VkPhysicalDevice);
		bool checkDeviceExtensionSupport(VkPhysicalDevice);

		struct SwapChainSupportDetails {
			VkSurfaceCapabilitiesKHR _capabilities;
			std::vector<VkSurfaceFormatKHR> _formats;
			std::vector<VkPresentModeKHR> _presentModes;
		};

		SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice);
		VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
		VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
		VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

		void loop();
		void drawFrame();
		void clean();

		static std::vector<char> readFile(const std::string& filename);
	};
}
