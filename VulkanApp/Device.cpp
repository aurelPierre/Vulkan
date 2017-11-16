#include "Device.h"

#include <map>
#include <set>

#include "Logging.h"

void Device::init(VkInstance instance, VkSurfaceKHR surface)
{
	pickPhysicalDevice(instance, surface);
	_indices = findQueueFamilies(_physicalDevice, surface);
	_swapChainSupportDetails = querySwapChainSupport(_physicalDevice, surface);
	createLogicalDevice(instance);
}

void Device::clean()
{
	vkDestroyDevice(_device, nullptr);
}

void Device::pickPhysicalDevice(VkInstance instance, VkSurfaceKHR surface)
{
	uint32_t deviceCount;
	vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

	if (!deviceCount)
		THROW("failed to find GPUs with Vulkan support")

	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

	const auto rateDevice = [](VkPhysicalDevice device) -> uint32_t {
		uint32_t score = 0;

		VkPhysicalDeviceProperties deviceProperties;
		VkPhysicalDeviceFeatures deviceFeatures;
		vkGetPhysicalDeviceProperties(device, &deviceProperties);
		vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

		if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
			score += 1000;

		score += deviceProperties.limits.maxImageDimension2D;

		if (!deviceFeatures.geometryShader)
			return 0;

		LOG(LogInfo, "rating " << score << util::Log::tab << deviceProperties.deviceName)

		return score;
	};

	std::map<uint32_t, VkPhysicalDevice> scoredDevices;
	for (const auto& device : devices)
		if (isDeviceSuitable(device, surface))
			scoredDevices.insert(std::make_pair(rateDevice(device), device));

	if (!scoredDevices.empty() && scoredDevices.rbegin()->first > 0)
		_physicalDevice = scoredDevices.rbegin()->second;
	else
		THROW("failed to find a suitable GPU")
}

void Device::createLogicalDevice(VkInstance instance)
{
	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<int> uniqueQueueFamilies = { _indices.graphicsFamily, _indices.presentFamily };

	float queuePriority = 1.f;
	for (int queueFamily : uniqueQueueFamilies) {
		VkDeviceQueueCreateInfo queueCreateInfo = {};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamily;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;
		queueCreateInfos.push_back(queueCreateInfo);
	}

	VkPhysicalDeviceFeatures deviceFeatures = {};
	VkDeviceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

	createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
	createInfo.pQueueCreateInfos = queueCreateInfos.data();

	createInfo.pEnabledFeatures = &deviceFeatures;
	createInfo.enabledLayerCount = 0;
	createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
	createInfo.ppEnabledExtensionNames = deviceExtensions.data();

	VkResult result = vkCreateDevice(_physicalDevice, &createInfo, nullptr, &_device);
	if (result != VK_SUCCESS)
		THROW("failed to create logical device with error :" + result)

	vkGetDeviceQueue(_device, _indices.graphicsFamily, 0, &_graphicsQueue);
	vkGetDeviceQueue(_device, _indices.presentFamily, 0, &_presentQueue);
}

bool Device::isDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface)
{
	QueueFamilyIndices indices = findQueueFamilies(device, surface);

	bool extensionsSupported = checkDeviceExtensionSupport(device);

	bool swapChainAdequate = false;
	if (extensionsSupported) {
		SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device, surface);
		swapChainAdequate = !swapChainSupport._formats.empty() && !swapChainSupport._presentModes.empty();
	}

	return indices.isComplete() && extensionsSupported && swapChainAdequate;
}

Device::QueueFamilyIndices Device::findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface)
{
	QueueFamilyIndices indices;

	uint32_t queueFamilyCount;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

	uint32_t i = 0;
	for (const auto& queueFamily : queueFamilies) {
		if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
			indices.graphicsFamily = i;

		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

		if (queueFamily.queueCount > 0 && presentSupport)
			indices.presentFamily = i;

		if (indices.isComplete())
			break;

		++i;
	}

	return indices;
}

bool Device::checkDeviceExtensionSupport(VkPhysicalDevice device)
{
	uint32_t extensionCount;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

	LOG(LogInfo, "available device extensions:")
	std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());
	for (const auto& extension : availableExtensions) {
		LOG(LogInfo, util::Log::tab << extension.extensionName)
		requiredExtensions.erase(extension.extensionName);
	}

	LOG(LogInfo, "required device extensions not supported:")
	for (const auto& extension : requiredExtensions)
		LOG(LogError, util::Log::tab << extension)

	return requiredExtensions.empty();
}

Device::SwapChainSupportDetails Device::querySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface)
{
	SwapChainSupportDetails details;

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details._capabilities);

	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
	if (formatCount != 0) {
		details._formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details._formats.data());
	}

	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);
	if (formatCount != 0) {
		details._presentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details._presentModes.data());
	}

	return details;
}
