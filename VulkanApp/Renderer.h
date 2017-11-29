#pragma once

#include <glm\glm.hpp>
#include <vulkan\vulkan.h>
#include <vector>

#include "NonCopyable.h"
#include "Device.h"
#include "RenderPass.h"


class Renderer : public util::NonCopyable
{
private:
	const std::vector<RenderPass::Vertex> vertices = {
		{ {-0.5f, -0.5f, -0.5f },	{ 1.f, 0.f, 0.f },	{ 0.f, 0.f } },
		{ {-0.5f, -0.5f, 0.5f },	{ 0.f, 1.f, 0.f },	{ 0.f, 0.f } },
		{ {-0.5f, 0.5f, -0.5f },	{ 0.f, 0.f, 1.f },	{ 0.f, 1.f } },
		{ {-0.5f, 0.5f, 0.5f },		{ 1.f, 1.f, 1.f },	{ 0.f, 1.f } },
		{ { 0.5f, -0.5f, -0.5f },	{ 1.f, 0.f, 0.f },	{ 1.f, 0.f } },
		{ { 0.5f, -0.5f, 0.5f },	{ 0.f, 1.f, 0.f },	{ 1.f, 0.f } },
		{ { 0.5f, 0.5f, -0.5f },	{ 0.f, 0.f, 1.f },	{ 1.f, 1.f } },
		{ { 0.5f, 0.5f, 0.5f },		{ 1.f, 1.f, 1.f },	{ 1.f, 1.f } },
	};


	const std::vector<uint16_t> indices = {
		0, 6, 4,
		0, 2, 6,
		0, 3, 2,
		0, 1, 3,
		2, 7, 6,
		2, 3, 7,
		4, 6, 7,
		4, 7, 5,
		0, 4, 5,
		0, 5, 1,
		1, 5, 7,
		1, 7, 3
	};

	struct Image {
		VkImage _image;
		VkImageView _imageView;
		VkFramebuffer _framebuffer;
		VkCommandBuffer _commandBuffer;
	};

public:
	Renderer() = default;
	~Renderer() = default;

	void init(const Device&, VkSurfaceKHR);
	void initImages(const Device&);
	void clean(const Device&);
	void recreate(const Device&, VkSurfaceKHR);

	void update(const Device&, float);
	void draw(const Device&);

	VkSwapchainKHR getSwapchainKHR() const { return _swapChain; }
	VkFormat getFormat() const { return _swapChainImageFormat; }
	const VkExtent2D& getExtent() const { return _swapChainExtent; }
	const VkCommandBuffer& getCommandBuffer(uint32_t index) { return _images[index]._commandBuffer; }

private:
	void createSwapChain(const Device&, VkSurfaceKHR);
	void createImageViews(const Device&);

	void createFramebuffers(const Device&);
	void createCommandPool(const Device&);
	void createCommandBuffers(const Device&);
	
	void createDepthResources(const Device&);
	bool hasStencilComponent(VkFormat);

	VkCommandBuffer beginSingleTimeCommands(const Device&);
	void endSingleTimeCommands(const Device&, VkCommandBuffer);

	void createTextureImage(const Device&);
	void createImage(const Device&, uint32_t width, uint32_t height, VkFormat, VkImageTiling, VkImageUsageFlags
					, VkMemoryPropertyFlags, VkImage&, VkDeviceMemory&);
	void transitionImageLayout(const Device&, VkImage, VkFormat, VkImageLayout oldLayout, VkImageLayout newLayout);
	void copyBufferToImage(const Device&, VkBuffer, VkImage, uint32_t width, uint32_t height);
	void createTextureImageView(const Device&);
	void createTextureSampler(const Device&);

	VkImageView createImageView(const Device&, VkImage, VkFormat, VkImageAspectFlags aspectFlags);

	void createUniformBuffer(const Device&);
	void createDescriptorPool(const Device&);
	void createDescriptorSet(const Device&);
	void createIndexBuffer(const Device&);
	void createVertexBuffer(const Device&);
	void createBuffer(const Device&, VkDeviceSize, VkBufferUsageFlags, VkMemoryPropertyFlags, VkBuffer&, VkDeviceMemory&);
	void copyBuffer(const Device&, VkBuffer src, VkBuffer dst, VkDeviceSize);

	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

	VkSwapchainKHR _swapChain;
	VkFormat _swapChainImageFormat;
	VkExtent2D _swapChainExtent;

	RenderPass _renderPass;

	VkCommandPool _commandPool;
	std::vector<Image> _images;

	VkBuffer _vertexBuffer;
	VkDeviceMemory _vertexBufferMemory;

	VkBuffer _indexBuffer;
	VkDeviceMemory _indexBufferMemory;

	VkBuffer _uniformBuffer;
	VkDeviceMemory _uniformBufferMemory;

	VkImage _textureImage;
	VkDeviceMemory _textureImageMemory;
	VkImageView _textureImageView;
	VkSampler _textureSampler;

	VkImage _depthImage;
	VkDeviceMemory _depthImageMemory;
	VkImageView _depthImageView;

	VkDescriptorPool _descriptorPool;
	VkDescriptorSet _descriptorSet;

	VkQueue _graphicsQueue;
	VkQueue _presentQueue;

	VkSemaphore _imageAvailableSemaphore;
	VkSemaphore _renderFinishedSemaphore;
};

