#include "scene.h"

unordered_map<string, shared_ptr<Mesh>>		Scene::s_meshes;
unordered_map<string, shared_ptr<Shader>>	Scene::s_shaders;
unordered_map<string, shared_ptr<Texture>>	Scene::s_textures;

Scene::Scene() : m_viewport{ 0.0f, 0.0f, static_cast<float>(Setting::SCREEN_WIDTH), static_cast<float>(Setting::SCREEN_HEIGHT), 0.0f, 1.0f }, m_scissorRect{ 0, 0, Setting::SCREEN_WIDTH, Setting::SCREEN_HEIGHT }
{

}