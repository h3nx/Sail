#include "Scene.h"
#include "../entities/components/Components.h"
#include "geometry/Model.h"
#include "light/LightSetup.h"
#include "../utils/Utils.h"

Scene::Scene() 
	: m_postProcessPass(m_renderer)
{

	// TODO: the following method ish
	// m_postProcessPass.add(chain index / prev link)

	auto window = Application::getInstance()->getWindow();
	UINT width = window->getWindowWidth();
	UINT height = window->getWindowHeight();

	m_deferredOutputTex = std::unique_ptr<RenderableTexture>(new RenderableTexture(1U, width, height, false));

}

Scene::~Scene() {

}

void Scene::addEntity(Entity::Ptr entity) {
	m_entities.push_back(MOVE(entity));
}

void Scene::setLightSetup(LightSetup* lights) {
	m_renderer.setLightSetup(lights);
}

void Scene::draw(Camera& camera) {

	m_renderer.begin(&camera);

	for (Entity::Ptr& entity : m_entities) {
		ModelComponent* model = entity->getComponent<ModelComponent>();
		if (model) {
			TransformComponent* transform = entity->getComponent<TransformComponent>();
			if (!transform)	Logger::Error("Tried to draw entity that is missing a TransformComponent!");

			m_renderer.submit(model->getModel(), transform->getTransform().getMatrix());
		}
	}

	m_renderer.end();
	m_renderer.present(m_deferredOutputTex.get());

	m_postProcessPass.run(*m_deferredOutputTex, nullptr);

	// Draw text last
	// TODO: sort entity list instead of iterating entire list twice
	for (Entity::Ptr& entity : m_entities) {
		TextComponent* text = entity->getComponent<TextComponent>();
		if (text) {
			text->draw();
		}
	}
}

void Scene::onEvent(Event& event) {
	// Forward events
	m_renderer.onEvent(event);
}
