#pragma once

#ifndef _TEXTURE_H_
#define _TEXTURE_H_

#include <vulkan/vulkan.h>

#include "Device.h"

class Texture
{
public:
	Texture(std::string);
	~Texture();

// TODO make it private
public:
	void createTextureImage();
	void createTextureImageView();
	void createTextureSampler();

	std::string _name;

	VkImage _textureImage;
	VkDeviceMemory _textureImageMemory;
	VkImageView _textureImageView;
	VkSampler _textureSampler;
};

#endif
