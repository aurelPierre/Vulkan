#pragma once

#ifndef _RESOURCE_MANAGER_H_
#define _RESOURCE_MANAGER_H_

#include <unordered_map>

#include "Model.h"
#include "Texture.h"

class ResourceManager
{
public:
	ResourceManager() = default;
	~ResourceManager() = default;

	void clean();

	void loadModel(std::string);
	Model* getModel(std::string) const;

	void loadTexture(std::string);
	Texture* getTexture(std::string) const;

private:
	std::unordered_map<std::string, Model*> _models;
	std::unordered_map<std::string, Texture*> _textures;
};

#endif // !_RESOURCE_MANAGER_H_


