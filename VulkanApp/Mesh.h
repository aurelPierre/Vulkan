#pragma once

#ifndef _MESH_H_
#define _MESH_H_

#include "Model.h"
#include "Texture.h"
#include "Device.h"

class Mesh
{
public:
	Mesh(Model*, Texture*);
	~Mesh();

	void init(const Device&);
	void clean(const Device&);

	void update(const Device&, float, float);
// TODO change to private
public:
	Model* _model;
	Texture* _texture;

	VkBuffer _uniformBuffer;
	VkDeviceMemory _uniformBufferMemory;

	VkDescriptorSet _descriptorSet;
	std::vector<VkCommandBuffer> _commandBuffers;
};

#endif // !_MESH_H_
