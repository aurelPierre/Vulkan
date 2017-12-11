#include "ResourceManager.h"

void ResourceManager::clean()
{
	for (auto& pair : _models)
		delete pair.second;

	for (auto& pair : _textures)
		delete pair.second;
}

void ResourceManager::loadModel(std::string file)
{
	if (_models.find(file) == _models.cend())
		_models[file] = new Model(file .c_str());
}

Model* ResourceManager::getModel(std::string name) const
{
	return _models.at(name);
}

void ResourceManager::loadTexture(std::string file)
{
	if (_textures.find(file) == _textures.cend())
		_textures[file] = new Texture(file.c_str());
}

Texture * ResourceManager::getTexture(std::string name) const
{
	return _textures.at(name);
}
