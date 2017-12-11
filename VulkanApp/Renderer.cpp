#include "Renderer.h"

#include <chrono>
#include <unordered_map>

#include "Logging.h"
#include "App.h"

void Renderer::init(const Device& device, VkSurfaceKHR surface)
{
	vkGetDeviceQueue(core::App::instance().getDevice().getDevice(), core::App::instance().getDevice().getIndices().presentFamily, 0, &_presentQueue);
	vkGetDeviceQueue(core::App::instance().getDevice().getDevice(), device.getIndices().presentFamily, 0, &_graphicsQueue);

	createSwapChain(device, surface);
	_renderPass.init(device, _swapChainImageFormat, _swapChainExtent);
	initImages(device);
}

void Renderer::initImages(const Device& device)
{
	createImageViews(device);
	createCommandPool(device);
	createDescriptorPool(device);
	createDepthResources(device);
	createFramebuffers(device);

	createPCommandBuffers(device);

	VkSemaphoreCreateInfo semaphoreInfo = {};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkResult result = vkCreateSemaphore(core::App::instance().getDevice().getDevice(), &semaphoreInfo, nullptr, &_imageAvailableSemaphore);
	if (result != VK_SUCCESS)
		THROW("failed to create semaphore with error: " + std::to_string(result))

	result = vkCreateSemaphore(core::App::instance().getDevice().getDevice(), &semaphoreInfo, nullptr, &_renderFinishedSemaphore);
	if (result != VK_SUCCESS)
		THROW("failed to create semaphore with error: " + std::to_string(result))
}


void Renderer::clean()
{
	vkDestroySemaphore(core::App::instance().getDevice().getDevice(), _renderFinishedSemaphore, nullptr);
	vkDestroySemaphore(core::App::instance().getDevice().getDevice(), _imageAvailableSemaphore, nullptr);

	vkDestroyDescriptorPool(core::App::instance().getDevice().getDevice(), _descriptorPool, nullptr);

	vkDestroyImageView(core::App::instance().getDevice().getDevice(), _depthImageView, nullptr);
	vkDestroyImage(core::App::instance().getDevice().getDevice(), _depthImage, nullptr);
	vkFreeMemory(core::App::instance().getDevice().getDevice(), _depthImageMemory, nullptr);

	vkDestroyCommandPool(core::App::instance().getDevice().getDevice(), _commandPool, nullptr);

	for (auto& image : _images)
		vkDestroyFramebuffer(core::App::instance().getDevice().getDevice(), image._framebuffer, nullptr);

	_renderPass.clean(core::App::instance().getDevice());

	for (auto& image : _images)
		vkDestroyImageView(core::App::instance().getDevice().getDevice(), image._imageView, nullptr);

	vkDestroySwapchainKHR(core::App::instance().getDevice().getDevice(), _swapChain, nullptr);
	_images.clear();
}

void Renderer::recreate(const Device& device, VkSurfaceKHR surface)
{
	vkDeviceWaitIdle(core::App::instance().getDevice().getDevice());
 	//clean(core::App::instance().getDevice()); // TODO clean this up
	init(device, surface);
}

void Renderer::update(const Device& device, float deltaTime)
{
	/*for (auto& mesh : _meshs)*/
		_meshs[0]->update(device, deltaTime, -0.5f);
		_meshs[1]->update(device, deltaTime, 0.5f);
}

void Renderer::draw(const Device& device)
{
	vkQueueWaitIdle(_presentQueue);

	uint32_t imageIndex;
	VkResult result = vkAcquireNextImageKHR(core::App::instance().getDevice().getDevice(), _swapChain, std::numeric_limits<uint64_t>::max(), _imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);
	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		util::Singleton<core::App>::instance().resize();
		return;
	} else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
		THROW("failed to acquire swap chain image with error: " + std::to_string(result))


	VkSemaphore waitSemaphores[] = { _imageAvailableSemaphore };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	std::vector<VkCommandBuffer> commandBuffers;
	for (const auto& mesh : _meshs)
		commandBuffers.emplace_back(mesh->_commandBuffers[imageIndex]);

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &_images[imageIndex]._primaryCommandBuffer;

	VkSemaphore signalSemaphores[] = { _renderFinishedSemaphore };
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	result = vkQueueSubmit(_graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	if (result != VK_SUCCESS)
		THROW("failed to submit draw command buffer with error: " + std::to_string(result))

	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;

	VkSwapchainKHR swapChains[] = { _swapChain };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;
	presentInfo.pImageIndices = &imageIndex;
	presentInfo.pResults = nullptr;

	result = vkQueuePresentKHR(_presentQueue, &presentInfo);
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
		util::Singleton<core::App>::instance().resize();
	else if(result != VK_SUCCESS)
		THROW("failed to present swap chain image with error: " + std::to_string(result))
}

void Renderer::addMesh(Mesh* mesh)
{
	_meshs.push_back(mesh);
	initMesh(core::App::instance().getDevice(), mesh);

	for (size_t i = 0; i < _images.size(); ++i) {
		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
		beginInfo.pInheritanceInfo = nullptr;

		vkBeginCommandBuffer(_images[i]._primaryCommandBuffer, &beginInfo);

		VkRenderPassBeginInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = _renderPass.getRenderPass();
		renderPassInfo.framebuffer = _images[i]._framebuffer;
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = _swapChainExtent;

		std::array<VkClearValue, 2> clearValues = {};
		clearValues[0].color = { 0.15f, 0.15f, 0.15f, 1.f };
		clearValues[1].depthStencil = { 1.f, 0 };

		renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
		renderPassInfo.pClearValues = clearValues.data();

		vkCmdBeginRenderPass(_images[i]._primaryCommandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);
		vkCmdBindPipeline(_images[i]._primaryCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _renderPass.getPipeline());

		std::vector<VkCommandBuffer> secondaryBuffers;
		for (const auto& mesh : _meshs)
			secondaryBuffers.emplace_back(mesh->_commandBuffers[i]);
		vkCmdExecuteCommands(_images[i]._primaryCommandBuffer, static_cast<uint32_t>(secondaryBuffers.size()), secondaryBuffers.data());

		vkCmdEndRenderPass(_images[i]._primaryCommandBuffer);

		VkResult result = vkEndCommandBuffer(_images[i]._primaryCommandBuffer);
		if (result != VK_SUCCESS)
			THROW("failed to record command buffer with error: " + std::to_string(result))
	}
}

void Renderer::initMesh(const Device& device, Mesh* mesh)
{
	createUniformBuffer(device, mesh);
		
	createDescriptorSet(device, mesh);
	createCommandBuffers(device, mesh);
}

void Renderer::createSwapChain(const Device& device, VkSurfaceKHR surface)
{
	Device::SwapChainSupportDetails swapChainSupport = device.querySwapChainSupport(device.getPhysicalDevice(), surface);

	VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport._formats);
	VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport._presentModes);
	VkExtent2D extent = chooseSwapExtent(swapChainSupport._capabilities);

	uint32_t imageCount = swapChainSupport._capabilities.minImageCount + 1;
	if (swapChainSupport._capabilities.maxImageCount > 0 && imageCount > swapChainSupport._capabilities.maxImageCount)
		imageCount = swapChainSupport._capabilities.maxImageCount;

	VkSwapchainCreateInfoKHR createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = surface;
	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	Device::QueueFamilyIndices indices = device.getIndices();
	uint32_t queueFamilyIndices[] = { (uint32_t)indices.graphicsFamily, (uint32_t)indices.presentFamily };
	if (indices.graphicsFamily != indices.presentFamily) {
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queueFamilyIndices;
	}
	else {
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		createInfo.queueFamilyIndexCount = 0;
		createInfo.pQueueFamilyIndices = nullptr;
	}

	createInfo.preTransform = swapChainSupport._capabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode = presentMode;
	createInfo.clipped = VK_TRUE;
	createInfo.oldSwapchain = VK_NULL_HANDLE;

	VkResult result = vkCreateSwapchainKHR(core::App::instance().getDevice().getDevice(), &createInfo, nullptr, &_swapChain);
	if (result != VK_SUCCESS)
		THROW("failed to create swap chain with error:" + std::to_string(result))

	_swapChainImageFormat = surfaceFormat.format;
	_swapChainExtent = extent;

	vkGetSwapchainImagesKHR(core::App::instance().getDevice().getDevice(), _swapChain, &imageCount, nullptr);
	_images.resize(imageCount);
	std::vector<VkImage> images(imageCount);
	vkGetSwapchainImagesKHR(core::App::instance().getDevice().getDevice(), _swapChain, &imageCount, images.data());

	for (size_t i = 0; i < images.size(); ++i)
		_images[i]._image = images[i];
}

void Renderer::createImageViews(const Device& device)
{
	for (size_t i = 0; i < _images.size(); ++i)
		_images[i]._imageView = createImageView(device, _images[i]._image, _swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);
}

void Renderer::createFramebuffers(const Device& device)
{
	for (size_t i = 0; i < _images.size(); ++i) {
		std::array<VkImageView, 2> attachments = {
			_images[i]._imageView,
			_depthImageView
		};

		VkFramebufferCreateInfo framebufferInfo = {};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = _renderPass.getRenderPass();
		framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		framebufferInfo.pAttachments = attachments.data();
		framebufferInfo.width = _swapChainExtent.width;
		framebufferInfo.height = _swapChainExtent.height;
		framebufferInfo.layers = 1;

		VkResult result = vkCreateFramebuffer(core::App::instance().getDevice().getDevice(), &framebufferInfo, nullptr, &_images[i]._framebuffer);
		if (result != VK_SUCCESS)
			THROW("failed to create framebuffer with error: " + std::to_string(result))
	}
}

void Renderer::createCommandPool(const Device& device)
{
	Device::QueueFamilyIndices queueFamilyIndices = device.getIndices();

	VkCommandPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily;
	poolInfo.flags = 0;

	VkResult result = vkCreateCommandPool(core::App::instance().getDevice().getDevice(), &poolInfo, nullptr, &_commandPool);
	if (result != VK_SUCCESS)
		THROW("failed to create command pool with error: " + std::to_string(result))
}

void Renderer::createCommandBuffers(const Device& device, Mesh* mesh)
{
	mesh->_commandBuffers.resize(_images.size());

	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = _commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
	allocInfo.commandBufferCount = static_cast<uint32_t>(mesh->_commandBuffers.size());

	VkResult result = vkAllocateCommandBuffers(core::App::instance().getDevice().getDevice(), &allocInfo, mesh->_commandBuffers.data());
	if (result != VK_SUCCESS)
		THROW("failed to allocate command buffers with error: " + std::to_string(result))

	for (size_t i = 0; i < _images.size(); ++i) {
		VkCommandBufferInheritanceInfo inheritanceInfo = {};
		inheritanceInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
		inheritanceInfo.renderPass = _renderPass.getRenderPass();
		inheritanceInfo.subpass = 1;
		inheritanceInfo.framebuffer = _images[i]._framebuffer;

		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
		beginInfo.pInheritanceInfo = &inheritanceInfo;

		vkBeginCommandBuffer(mesh->_commandBuffers[i], &beginInfo);
		vkCmdBindPipeline(mesh->_commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, _renderPass.getPipeline());

		VkBuffer vertexBuffers[] = { mesh->_model->_vertexBuffer };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(mesh->_commandBuffers[i], 0, 1, vertexBuffers, offsets);
		vkCmdBindIndexBuffer(mesh->_commandBuffers[i], mesh->_model->_indexBuffer, 0, VK_INDEX_TYPE_UINT32);
		vkCmdBindDescriptorSets(mesh->_commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, _renderPass.getPipelineLayout(), 0, 1,
			&mesh->_descriptorSet, 0, nullptr);

		vkCmdDrawIndexed(mesh->_commandBuffers[i], static_cast<uint32_t>(mesh->_model->_indices.size()), 1, 0, 0, 0);

		result = vkEndCommandBuffer(mesh->_commandBuffers[i]);
		if (result != VK_SUCCESS)
			THROW("failed to record command buffer with error: " + std::to_string(result))
	}
}

void Renderer::createDepthResources(const Device& device)
{
	VkFormat depthFormat = _renderPass.findDepthFormat(device);
	createImage(device, _swapChainExtent.width, _swapChainExtent.height, depthFormat, VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, _depthImage, _depthImageMemory);
	_depthImageView = createImageView(device, _depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
	transitionImageLayout(device, _depthImage, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
}


bool Renderer::hasStencilComponent(VkFormat format)
{
	return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

VkCommandBuffer Renderer::beginSingleTimeCommands(const Device& device)
{
	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = _commandPool;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(core::App::instance().getDevice().getDevice(), &allocInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(commandBuffer, &beginInfo);
	return commandBuffer;
}

void Renderer::endSingleTimeCommands(const Device& device, VkCommandBuffer commandBuffer)
{
	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	vkQueueSubmit(_graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(_graphicsQueue);

	vkFreeCommandBuffers(core::App::instance().getDevice().getDevice(), _commandPool, 1, &commandBuffer);
}

void Renderer::createImage(const Device& device, uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, 
						VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory)
{
	VkImageCreateInfo imageInfo = {};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width = width;
	imageInfo.extent.height = height;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.format = format;
	imageInfo.tiling = tiling;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = usage;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.flags = 0;

	VkResult result = vkCreateImage(core::App::instance().getDevice().getDevice(), &imageInfo, nullptr, &image);
	if (result != VK_SUCCESS)
		THROW("failed to create image with error: " + std::to_string(result))

	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(core::App::instance().getDevice().getDevice(), image, &memRequirements);

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = device.findMemoryType(memRequirements.memoryTypeBits, properties);

	result = vkAllocateMemory(core::App::instance().getDevice().getDevice(), &allocInfo, nullptr, &imageMemory);
	if (result != VK_SUCCESS)
		THROW("failed to allocate image memory with error: " + std::to_string(result))

	vkBindImageMemory(core::App::instance().getDevice().getDevice(), image, imageMemory, 0);
}

void Renderer::transitionImageLayout(const Device& device, VkImage image, VkFormat format, VkImageLayout oldLayout
									, VkImageLayout newLayout)
{
	VkCommandBuffer commandBuffer = beginSingleTimeCommands(device);

	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.srcAccessMask = 0;
	barrier.dstAccessMask = 0;

	if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		if (hasStencilComponent(format))
			barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
	} else {
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	}

	VkPipelineStageFlags srcStage;
	VkPipelineStageFlags dstStage;
	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	} else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	} else if(oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		dstStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	} else {
		THROW("unsupported layout transitions")
	}

	vkCmdPipelineBarrier(commandBuffer, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

	endSingleTimeCommands(device, commandBuffer);
}

void Renderer::copyBufferToImage(const Device& device, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
{
	VkCommandBuffer commandBuffer = beginSingleTimeCommands(device);

	VkBufferImageCopy region = {};
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;

	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;

	region.imageOffset = { 0, 0, 0 };
	region.imageExtent = { width, height, 1 };

	vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

	endSingleTimeCommands(device, commandBuffer);
}

VkImageView Renderer::createImageView(const Device& device, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags)
{
	VkImageViewCreateInfo viewInfo = {};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = image;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = format;
	viewInfo.subresourceRange.aspectMask = aspectFlags;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;

	VkImageView imageView;
	VkResult result = vkCreateImageView(core::App::instance().getDevice().getDevice(), &viewInfo, nullptr, &imageView);
	if (result != VK_SUCCESS)
		THROW("failed to create texture image view with error: " + std::to_string(result))

	return imageView;
}

void Renderer::createUniformBuffer(const Device& device, Mesh* mesh)
{
	VkDeviceSize bufferSize = sizeof(RenderPass::UniformBufferObject);
	createBuffer(device, bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
		| VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, mesh->_uniformBuffer, mesh->_uniformBufferMemory);
}

void Renderer::createDescriptorPool(const Device& device)
{
	std::array<VkDescriptorPoolSize, 2> poolSizes = {};
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[0].descriptorCount = 1;
	poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[1].descriptorCount = 1;

	VkDescriptorPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	poolInfo.pPoolSizes = poolSizes.data();
	poolInfo.maxSets = 2;

	VkResult result = vkCreateDescriptorPool(core::App::instance().getDevice().getDevice(), &poolInfo, nullptr, &_descriptorPool);
	if(result != VK_SUCCESS)
		THROW("failed to create descriptor pool with error: " + std::to_string(result))
}

void Renderer::createDescriptorSet(const Device& device, Mesh* mesh)
{
	VkDescriptorSetLayout layouts[] = { _renderPass.getDescriptorSetLayout() };
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = _descriptorPool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = layouts;

	VkResult result = vkAllocateDescriptorSets(core::App::instance().getDevice().getDevice(), &allocInfo, &mesh->_descriptorSet);
	if (result != VK_SUCCESS)
		THROW("failed to allocate descriptor set with error: " + std::to_string(result))

	VkDescriptorBufferInfo bufferInfo = {};
	bufferInfo.buffer = mesh->_uniformBuffer;
	bufferInfo.offset = 0;
	bufferInfo.range = sizeof(RenderPass::UniformBufferObject);

	VkDescriptorImageInfo imageInfo = {};
	imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	imageInfo.imageView = mesh->_texture->_textureImageView;
	imageInfo.sampler = mesh->_texture->_textureSampler;

	std::array<VkWriteDescriptorSet, 2> descriptorWrites = {};
	descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[0].dstSet = mesh->_descriptorSet;
	descriptorWrites[0].dstBinding = 0;
	descriptorWrites[0].dstArrayElement = 0;
	descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorWrites[0].descriptorCount = 1;
	descriptorWrites[0].pBufferInfo = &bufferInfo;

	descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[1].dstSet = mesh->_descriptorSet;
	descriptorWrites[1].dstBinding = 1;
	descriptorWrites[1].dstArrayElement = 0;
	descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrites[1].descriptorCount = 1;
	descriptorWrites[1].pImageInfo = &imageInfo;

	vkUpdateDescriptorSets(core::App::instance().getDevice().getDevice(), static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
}

void Renderer::createBuffer(const Device& device, VkDeviceSize size, VkBufferUsageFlags usage, 
							VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& memory)
{
	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VkResult result = vkCreateBuffer(core::App::instance().getDevice().getDevice(), &bufferInfo, nullptr, &buffer);
	if (result != VK_SUCCESS)
		THROW("failed to create vertex buffer with error: " + std::to_string(result))

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(core::App::instance().getDevice().getDevice(), buffer, &memRequirements);

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = device.findMemoryType(memRequirements.memoryTypeBits, properties);
	
	result = vkAllocateMemory(core::App::instance().getDevice().getDevice(), &allocInfo, nullptr, &memory);
	if (result != VK_SUCCESS)
		THROW("failed to allocate vertex buffer memory with errror: " + std::to_string(result))

	vkBindBufferMemory(core::App::instance().getDevice().getDevice(), buffer, memory, 0);
}

void Renderer::copyBuffer(const Device& device, VkBuffer src, VkBuffer dst, VkDeviceSize size)
{
	VkCommandBuffer commandBuffer = beginSingleTimeCommands(device);

	VkBufferCopy copyRegion = {};
	copyRegion.srcOffset = 0;
	copyRegion.dstOffset = 0;
	copyRegion.size = size;
	vkCmdCopyBuffer(commandBuffer, src, dst, 1, &copyRegion);

	endSingleTimeCommands(device, commandBuffer);
}

void Renderer::createPCommandBuffers(const Device& device)
{
	for (size_t i = 0; i < _images.size(); ++i) {
		_images[i]._primaryCommandBuffer = {};

		VkCommandBufferAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = _commandPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = 1;

		VkResult result = vkAllocateCommandBuffers(core::App::instance().getDevice().getDevice(), &allocInfo, &_images[i]._primaryCommandBuffer);
		if (result != VK_SUCCESS)
			THROW("failed to allocate command buffers with error: " + std::to_string(result))

		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
		beginInfo.pInheritanceInfo = nullptr;

		vkBeginCommandBuffer(_images[i]._primaryCommandBuffer, &beginInfo);

		VkRenderPassBeginInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = _renderPass.getRenderPass();
		renderPassInfo.framebuffer = _images[i]._framebuffer;
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = _swapChainExtent;

		std::array<VkClearValue, 2> clearValues = {};
		clearValues[0].color = { 0.15f, 0.15f, 0.15f, 1.f };
		clearValues[1].depthStencil = { 1.f, 0 };

		renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
		renderPassInfo.pClearValues = clearValues.data();

		vkCmdBeginRenderPass(_images[i]._primaryCommandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);
		vkCmdBindPipeline(_images[i]._primaryCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _renderPass.getPipeline());

		std::vector<VkCommandBuffer> secondaryBuffers;
		for (const auto& mesh : _meshs)
			secondaryBuffers.emplace_back(mesh->_commandBuffers[i]);
		vkCmdExecuteCommands(_images[i]._primaryCommandBuffer, static_cast<uint32_t>(secondaryBuffers.size()), secondaryBuffers.data());

		vkCmdEndRenderPass(_images[i]._primaryCommandBuffer);

		result = vkEndCommandBuffer(_images[i]._primaryCommandBuffer);
		if (result != VK_SUCCESS)
			THROW("failed to record command buffer with error: " + std::to_string(result))
	}
}

VkSurfaceFormatKHR Renderer::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
	if (availableFormats.size() == 1 && availableFormats[0].format == VK_FORMAT_UNDEFINED)
		return { VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };

	for (const auto& availableFormat : availableFormats)
		if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM
			&& availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			return availableFormat;

	return availableFormats[0];
}

VkPresentModeKHR Renderer::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
{
	VkPresentModeKHR bestMode = VK_PRESENT_MODE_FIFO_KHR;

	for (const auto& availablePresentMode : availablePresentModes) {
		if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
			return availablePresentMode;
		else if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR)
			bestMode = availablePresentMode;
	}

	return bestMode;
}

VkExtent2D Renderer::chooseSwapExtent(const VkSurfaceCapabilitiesKHR & capabilities)
{
	if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
		return capabilities.currentExtent;
	else {
		int width, height;
		util::Singleton<core::App>::instance().getWindowSize(width, height);
		VkExtent2D actualExtent = { width, height };

		actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
		actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
		return actualExtent;
	}
}
