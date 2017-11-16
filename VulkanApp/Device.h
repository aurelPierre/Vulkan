#pragma once

#include <vulkan\vulkan.h>
#include <vector>
#include <array>

#include "NonCopyable.h"

class Device : public util::NonCopyable
{
public:
	struct QueueFamilyIndices {
		int graphicsFamily = -1;
		int presentFamily = -1;

		bool isComplete() {
			return graphicsFamily >= 0 && presentFamily >= 0;
		}
	};

	struct SwapChainSupportDetails {
		VkSurfaceCapabilitiesKHR _capabilities;
		std::vector<VkSurfaceFormatKHR> _formats;
		std::vector<VkPresentModeKHR> _presentModes;
	};

	Device() = default;
	~Device() = default;

	void init(VkInstance, VkSurfaceKHR);
	void clean();

	VkDevice getDevice() const { return _device; }
	const QueueFamilyIndices& getIndices() const { return _indices; }
	const SwapChainSupportDetails& getSwapChainSupportDetails() const { return _swapChainSupportDetails; }

	VkQueue getGraphicsQueue() const { return _graphicsQueue; }
	VkQueue getPresentQueue() const { return _presentQueue; }

private:

	const std::array<const char*, 1> deviceExtensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};

	void pickPhysicalDevice(VkInstance, VkSurfaceKHR);
	void createLogicalDevice(VkInstance);

	bool isDeviceSuitable(VkPhysicalDevice, VkSurfaceKHR);
	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice, VkSurfaceKHR);
	bool checkDeviceExtensionSupport(VkPhysicalDevice);
	SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice, VkSurfaceKHR);

	VkPhysicalDevice _physicalDevice;
	VkDevice _device;
	QueueFamilyIndices _indices;
	SwapChainSupportDetails _swapChainSupportDetails;

	VkQueue _graphicsQueue;
	VkQueue _presentQueue;
};
