#include "pch.h"
#include "ResourceManager.h"
#include "Sail/api/shader/PipelineStateObject.h"
#include "../api/shader/Shader.h"

ResourceManager::ResourceManager() {
	{
		ShaderSettings settings;
		settings.filename = "PBRMaterialShader.hlsl";
		settings.materialType = Material::PBR;
		settings.defaultPSOSettings.cullMode = GraphicsAPI::BACKFACE;
		m_shaderSettings.insert({ PBRMaterialShader, settings });
	}
	{
		ShaderSettings settings;
		settings.filename = "PhongMaterialShader.hlsl";
		settings.materialType = Material::PHONG;
		settings.defaultPSOSettings.cullMode = GraphicsAPI::BACKFACE;
		m_shaderSettings.insert({ PhongMaterialShader, settings });
	}
	{
		ShaderSettings settings;
		settings.filename = "OutlineShader.hlsl";
		settings.materialType = Material::OUTLINE;
		settings.defaultPSOSettings.cullMode = GraphicsAPI::FRONTFACE;
		m_shaderSettings.insert({ OutlineShader, settings });
	}
	{
		ShaderSettings settings;
		settings.filename = "CubemapShader.hlsl";
		settings.materialType = Material::TEXTURES;
		settings.defaultPSOSettings.depthMask = GraphicsAPI::BUFFER_DISABLED;
		settings.defaultPSOSettings.cullMode = GraphicsAPI::FRONTFACE;
		m_shaderSettings.insert({ CubemapShader, settings });
	}
	{
		ShaderSettings settings;
		settings.filename = "compute/GenerateMipsCS.hlsl";
		settings.materialType = Material::NONE;
		settings.computeShaderSettings.threadGroupXScale = 1.0f / 8.0f;
		settings.computeShaderSettings.threadGroupYScale = 1.0f / 8.0f;
		m_shaderSettings.insert({ GenerateMipsComputeShader, settings });
	}
}
ResourceManager::~ResourceManager() {
	for (auto it : m_shaders) {
		delete it.second;
	}
}

//
// TextureData
//

void ResourceManager::loadTextureData(const std::string& filename, bool useAbsolutePath) {
	m_textureDatas.insert({ filename, std::make_unique<TextureData>(filename, useAbsolutePath) });
}
TextureData& ResourceManager::getTextureData(const std::string& filename) {
	auto pos = m_textureDatas.find(filename);
	if (pos == m_textureDatas.end())
		Logger::Error("Tried to access a resource that was not loaded. (" + filename + ") \n Use Application::getInstance()->getResourceManager().LoadTextureData(\"filename\") before accessing it.");

	return *pos->second;
}
bool ResourceManager::hasTextureData(const std::string& filename) {
	return m_textureDatas.find(filename) != m_textureDatas.end();
}

bool ResourceManager::releaseTextureData(const std::string& filename) {
	return m_textureDatas.erase(filename);
}

//
// DXTexture
//

void ResourceManager::loadTexture(const std::string& filename, bool useAbsolutePath) {
	SAIL_PROFILE_FUNCTION();

	if (!hasTexture(filename))
		m_textures.insert({ filename, std::unique_ptr<Texture>(Texture::Create(filename, useAbsolutePath)) });
}
Texture& ResourceManager::getTexture(const std::string& filename) {
	auto pos = m_textures.find(filename);
	if (pos == m_textures.end())
		Logger::Error("Tried to access a resource that was not loaded. (" + filename + ") \n Use Application::getInstance()->getResourceManager().loadTexture(\"" + filename + "\") before accessing it.");

	return *pos->second;
}
bool ResourceManager::hasTexture(const std::string& filename) {
	return m_textures.find(filename) != m_textures.end();
}


//
// Model
//

void ResourceManager::loadModel(const std::string& filename, Shader* shader, bool useAbsolutePath) {
	// Insert the new model
	m_fbxModels.insert({ filename, std::make_unique<ParsedScene>(filename, shader, useAbsolutePath) });
}
std::shared_ptr<Model> ResourceManager::getModel(const std::string& filename, Shader* shader, bool useAbsolutePath) {
	auto pos = m_fbxModels.find(filename);
	if (pos == m_fbxModels.end()) {
		// Model was not yet loaded, load it and return
		loadModel(filename, shader, useAbsolutePath);

		return m_fbxModels.find(filename)->second->getModel();
	}

	auto foundModel = pos->second->getModel();
	if (foundModel->getMesh(0)->getShader() != shader)
		Logger::Error("Tried to get model from resource manager that has already been loaded with a different shader!");

	return foundModel;
}
bool ResourceManager::hasModel(const std::string& filename) {
	return m_fbxModels.find(filename) != m_fbxModels.end();
}

void ResourceManager::loadShaderSet(ShaderIdentifier shaderIdentifier) {
	m_shaders.insert({ shaderIdentifier, Shader::Create(m_shaderSettings[shaderIdentifier]) });
}

Shader& ResourceManager::getShaderSet(ShaderIdentifier shaderIdentifier) {
	auto pos = m_shaders.find(shaderIdentifier);
	if (pos == m_shaders.end()) {
		// ShaderSet was not yet loaded, load it and return
		loadShaderSet(shaderIdentifier);
		return *m_shaders.find(shaderIdentifier)->second;
	}

	return *pos->second;
}

bool ResourceManager::hasShaderSet(ShaderIdentifier shaderIdentifier) {
	return m_shaders.find(shaderIdentifier) != m_shaders.end();
}

void ResourceManager::reloadShader(ShaderIdentifier shaderIdentifier) {
	auto it = m_shaders.find(shaderIdentifier);
	if (it == m_shaders.end()) {
		Logger::Log("Cannot reload shader " + std::to_string(shaderIdentifier) + " since it is not loaded in the first place.");
		return;
	}
	Shader* shader = it->second;
	shader->~Shader();
	//shader = new (shader) Shader::Create(m_shaderSettings[shaderIdentifier]);  // TODO: fix
	shader = Shader::Create(m_shaderSettings[shaderIdentifier]);
	Logger::Log("Reloaded shader " + std::to_string(shaderIdentifier));
}

PipelineStateObject& ResourceManager::getPSO(Shader* shader, Mesh* mesh) {
	unsigned int hash = 0;
	unsigned int meshHash = 0;
	if (mesh) {
		// Return a PSO that matches the attribute order in the mesh and the shader
		// The combination is hashed in a uint like "xxxxxyyyyy" where x is the shader id and y is the attribute hash
		meshHash = mesh->getAttributesHash();
		assert(shader->getID() < 21473 && "Too many Shader instances exist, how did that even happen?");
		hash = meshHash + shader->getID() * 10e5;
	} else {
		assert(shader->isComputeShader() && "A mesh has to be specified for all shader types except compute shaders when getting a PSO");
	}

	auto pos = m_psos.find(hash);
	if (pos == m_psos.end()) {
		// ShaderSet was not yet loaded, load it and return
		return *m_psos.insert({ hash, std::unique_ptr<PipelineStateObject>(PipelineStateObject::Create(shader, meshHash)) }).first->second;
	}

	return *pos->second;
}
