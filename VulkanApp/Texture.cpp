#include "Texture.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "Logging.h"
#include "App.h"

Texture::Texture(std::string file) : _name { file }
{
	createTextureImage();
	createTextureImageView();
	createTextureSampler();
}

Texture::~Texture()
{
	vkDestroySampler(core::App::instance().getDevice().getDevice(), _textureSampler, nullptr);
	vkDestroyImageView(core::App::instance().getDevice().getDevice(), _textureImageView, nullptr);

	vkDestroyImage(core::App::instance().getDevice().getDevice(), _textureImage, nullptr);
	vkFreeMemory(core::App::instance().getDevice().getDevice(), _textureImageMemory, nullptr);
}

void Texture::createTextureImage()
{
	int texWidth, texHeight, texChannels;
	stbi_uc* pixels = stbi_load(_name.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
	VkDeviceSize imageSize = texWidth * texHeight * 4;
	if (!pixels)
		THROW("failed to load texture image")

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	core::App::instance().getRenderer().createBuffer(core::App::instance().getDevice(), imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
		| VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);
	void* data;
	vkMapMemory(core::App::instance().getDevice().getDevice(), stagingBufferMemory, 0, imageSize, 0, &data);
	memcpy(data, pixels, static_cast<size_t>(imageSize));
	vkUnmapMemory(core::App::instance().getDevice().getDevice(), stagingBufferMemory);

	stbi_image_free(pixels);

	core::App::instance().getRenderer().createImage(core::App::instance().getDevice(), texWidth, texHeight, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT
		| VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, _textureImage, _textureImageMemory);
	core::App::instance().getRenderer().transitionImageLayout(core::App::instance().getDevice(), _textureImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_UNDEFINED
		, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	core::App::instance().getRenderer().copyBufferToImage(core::App::instance().getDevice(), stagingBuffer, _textureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
	core::App::instance().getRenderer().transitionImageLayout(core::App::instance().getDevice(), _textureImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
		, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	vkDestroyBuffer(core::App::instance().getDevice().getDevice(), stagingBuffer, nullptr);
	vkFreeMemory(core::App::instance().getDevice().getDevice(), stagingBufferMemory, nullptr);
}

void Texture::createTextureImageView()
{
	_textureImageView = core::App::instance().getRenderer().createImageView(core::App::instance().getDevice(), _textureImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT);
}

void Texture::createTextureSampler()
{
	VkSamplerCreateInfo samplerInfo = {};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.anisotropyEnable = VK_TRUE;
	samplerInfo.maxAnisotropy = 16;
	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.mipLodBias = 0.f;
	samplerInfo.minLod = 0.f;
	samplerInfo.maxLod = 0.f;

	VkResult result = vkCreateSampler(core::App::instance().getDevice().getDevice(), &samplerInfo, nullptr, &_textureSampler);
	if (result != VK_SUCCESS)
		THROW("failed to create texture sampler with error: " + std::to_string(result))
}
