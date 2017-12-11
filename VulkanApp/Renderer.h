#pragma once

#include <glm\glm.hpp>
#include <vulkan\vulkan.h>
#include <vector>

#include "NonCopyable.h"
#include "Device.h"
#include "RenderPass.h"
#include "Mesh.h"

class Renderer : public util::NonCopyable
{
private:
	struct Image {
		VkImage _image;
		VkImageView _imageView;
		VkFramebuffer _framebuffer;
		VkCommandBuffer _primaryCommandBuffer;
	};

public:
	Renderer() = default;
	~Renderer() = default;

	void init(const Device&, VkSurfaceKHR);
	void initImages(const Device&);
	void recreate(const Device&, VkSurfaceKHR);
	void clean();

	void update(const Device&, float);
	void draw(const Device&);

	VkSwapchainKHR getSwapchainKHR() const { return _swapChain; }
	VkFormat getFormat() const { return _swapChainImageFormat; }
	const VkExtent2D& getExtent() const { return _swapChainExtent; }

public:
	void createSwapChain(const Device&, VkSurfaceKHR);

	void createImageViews(const Device&);
	void createFramebuffers(const Device&);

	void createCommandPool(const Device&);
	void createDescriptorPool(const Device&);

	void createDepthResources(const Device&);
	bool hasStencilComponent(VkFormat);

	VkCommandBuffer beginSingleTimeCommands(const Device&);
	void endSingleTimeCommands(const Device&, VkCommandBuffer);

	static void createImage(const Device&, uint32_t width, uint32_t height, VkFormat, VkImageTiling, VkImageUsageFlags
					, VkMemoryPropertyFlags, VkImage&, VkDeviceMemory&);
	void transitionImageLayout(const Device&, VkImage, VkFormat, VkImageLayout oldLayout, VkImageLayout newLayout);
	void copyBufferToImage(const Device&, VkBuffer, VkImage, uint32_t width, uint32_t height);

	static VkImageView createImageView(const Device&, VkImage, VkFormat, VkImageAspectFlags aspectFlags);

	void addMesh(Mesh*);
	void initMesh(const Device&, Mesh*);
	void createUniformBuffer(const Device&, Mesh*);
	void createDescriptorSet(const Device&, Mesh*);
	void createCommandBuffers(const Device&, Mesh*);

	static void createBuffer(const Device&, VkDeviceSize, VkBufferUsageFlags, VkMemoryPropertyFlags, VkBuffer&, VkDeviceMemory&);
	void copyBuffer(const Device&, VkBuffer src, VkBuffer dst, VkDeviceSize);

	void createPCommandBuffers(const Device&);

	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

private:
	VkSwapchainKHR _swapChain;
	VkFormat _swapChainImageFormat;
	VkExtent2D _swapChainExtent;

	RenderPass _renderPass;

	VkCommandPool _commandPool;
	VkDescriptorPool _descriptorPool;

	std::vector<Image> _images;
	std::vector<Mesh*> _meshs;

	VkImage _depthImage;
	VkDeviceMemory _depthImageMemory;
	VkImageView _depthImageView;

	VkQueue _graphicsQueue;
	VkQueue _presentQueue;

	VkSemaphore _imageAvailableSemaphore;
	VkSemaphore _renderFinishedSemaphore;
};

