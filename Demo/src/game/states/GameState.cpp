#include "GameState.h"
#include "imgui.h"
#include "Sail/debug/Instrumentor.h"
#include "Sail/graphics/material/PhongMaterial.h"

GameState::GameState(StateStack& stack)
: State(stack)
, m_cam(90.f, 1280.f / 720.f, 0.1f, 5000.f)
, m_camController(&m_cam)
{
	SAIL_PROFILE_FUNCTION();

	// Get the Application instance
	m_app = Application::getInstance();
	//m_scene = std::make_unique<Scene>(AABB(glm::vec3(-100.f, -100.f, -100.f), glm::vec3(100.f, 100.f, 100.f)));

	// Textures needs to be loaded before they can be used
	// TODO: automatically load textures when needed so the following can be removed
	Application::getInstance()->getResourceManager().loadTexture("sponza/textures/spnza_bricks_a_ddn.tga");
	Application::getInstance()->getResourceManager().loadTexture("sponza/textures/spnza_bricks_a_diff.tga");
	Application::getInstance()->getResourceManager().loadTexture("sponza/textures/spnza_bricks_a_spec.tga");

	// Set up camera with controllers
	m_cam.setPosition(glm::vec3(1.6f, 4.7f, 7.4f));
	m_camController.lookAt(glm::vec3(0.f));
	
	// Add a directional light
	glm::vec3 color(1.0f, 1.0f, 1.0f);
 	glm::vec3 direction(0.4f, -0.2f, 1.0f);
	direction = glm::normalize(direction);
	m_lights.setDirectionalLight(DirectionalLight(color, direction));
	// Add four point lights
	{
		PointLight pl;
		pl.setAttenuation(.0f, 0.1f, 0.02f);
		pl.setColor(glm::vec3(Utils::rnd(), Utils::rnd(), Utils::rnd()));
		pl.setPosition(glm::vec3(-4.0f, 0.1f, -4.0f));
		m_lights.addPointLight(pl);

		pl.setColor(glm::vec3(Utils::rnd(), Utils::rnd(), Utils::rnd()));
		pl.setPosition(glm::vec3(-4.0f, 0.1f, 4.0f));
		m_lights.addPointLight(pl);

		pl.setColor(glm::vec3(Utils::rnd(), Utils::rnd(), Utils::rnd()));
		pl.setPosition(glm::vec3(4.0f, 0.1f, 4.0f));
		m_lights.addPointLight(pl);

		pl.setColor(glm::vec3(Utils::rnd(), Utils::rnd(), Utils::rnd()));
		pl.setPosition(glm::vec3(4.0f, 0.1f, -4.0f));
		m_lights.addPointLight(pl);
	}

	// Set up the scene
	//m_scene->addSkybox(L"skybox_space_512.dds"); //TODO
	m_scene.setLightSetup(&m_lights);

	// Disable culling for testing purposes
	m_app->getAPI()->setFaceCulling(GraphicsAPI::NO_CULLING);

	auto* shader = &m_app->getResourceManager().getShaderSet<PhongMaterialShader>();

	// Create/load models
	m_cubeModel = ModelFactory::CubeModel::Create(glm::vec3(0.5f), shader);
	m_planeModel = ModelFactory::PlaneModel::Create(glm::vec2(50.f), shader, glm::vec2(30.0f));
	
	
	Model* fbxModel = &m_app->getResourceManager().getModel("sphere.fbx", shader);

	// Create entities
	{
		auto e = Entity::Create("Static cube");
		e->addComponent<ModelComponent>(m_cubeModel.get());
		e->addComponent<TransformComponent>(glm::vec3(-4.f, 1.f, -2.f));
		auto* mat = e->addComponent<MaterialComponent>(Material::PHONG);
		mat->get()->asPhong()->setColor(glm::vec4(0.2f, 0.8f, 0.4f, 1.0f));
		m_scene.addEntity(e);
	}

	{
		auto e = Entity::Create("Floor");
		e->addComponent<ModelComponent>(m_planeModel.get());
		e->addComponent<TransformComponent>(glm::vec3(0.f, 0.f, 0.f));
		auto* mat = e->addComponent<MaterialComponent>(Material::PHONG);
		mat->get()->asPhong()->setDiffuseTexture("sponza/textures/spnza_bricks_a_diff.tga");
		mat->get()->asPhong()->setNormalTexture("sponza/textures/spnza_bricks_a_ddn.tga");
		mat->get()->asPhong()->setSpecularTexture("sponza/textures/spnza_bricks_a_spec.tga");
		m_scene.addEntity(e);
	}

	Entity::SPtr parentEntity;
	{
		parentEntity = Entity::Create("Clingy cube");
		parentEntity->addComponent<ModelComponent>(m_cubeModel.get());
		parentEntity->addComponent<TransformComponent>(glm::vec3(-1.2f, 1.f, -1.f), glm::vec3(0.f, 0.f, 1.07f));
		parentEntity->addComponent<MaterialComponent>(Material::PHONG);
		m_scene.addEntity(parentEntity);
	}

	{
		// Add some cubes which are connected through parenting
		m_texturedCubeEntity = Entity::Create("Textured parent cube");
		m_texturedCubeEntity->addComponent<ModelComponent>(fbxModel);
		m_texturedCubeEntity->addComponent<TransformComponent>(glm::vec3(-1.f, 2.f, 0.f), m_texturedCubeEntity->getComponent<TransformComponent>());
		auto* mat = m_texturedCubeEntity->addComponent<MaterialComponent>(Material::PHONG);
		mat->get()->asPhong()->setDiffuseTexture("sponza/textures/spnza_bricks_a_diff.tga");
		mat->get()->asPhong()->setNormalTexture("sponza/textures/spnza_bricks_a_ddn.tga");
		mat->get()->asPhong()->setSpecularTexture("sponza/textures/spnza_bricks_a_spec.tga");
		m_texturedCubeEntity->setName("MovingCube");
		m_scene.addEntity(m_texturedCubeEntity);
		parentEntity->getComponent<TransformComponent>()->setParent(m_texturedCubeEntity->getComponent<TransformComponent>());
	}

	{
		auto e = Entity::Create("CubeRoot");
		e->addComponent<ModelComponent>(m_cubeModel.get());
		e->addComponent<TransformComponent>(glm::vec3(10.f, 0.f, 10.f));
		e->addComponent<MaterialComponent>(Material::PHONG);
		m_scene.addEntity(e);
		m_transformTestEntities.push_back(e);
	}

	{
		auto e = Entity::Create("CubeChild");
		e->addComponent<ModelComponent>(m_cubeModel.get());
		e->addComponent<TransformComponent>(glm::vec3(1.f, 1.f, 1.f), m_transformTestEntities[0]->getComponent<TransformComponent>());
		e->addComponent<MaterialComponent>(Material::PHONG);
		m_scene.addEntity(e);
		m_transformTestEntities.push_back(e);
	}

	{
		auto e = Entity::Create("CubeChildChild");
		e->addComponent<ModelComponent>(m_cubeModel.get());
		e->addComponent<TransformComponent>(glm::vec3(1.f, 1.f, 1.f), m_transformTestEntities[1]->getComponent<TransformComponent>());
		e->addComponent<MaterialComponent>(Material::PHONG);
		m_scene.addEntity(e);
		m_transformTestEntities.push_back(e);
	}




	// Random cube maze
	const unsigned int mazeStart = 5;
	const unsigned int mazeSize = 20;
	const float wallSize = 1.1f;
	for (unsigned int x = 0; x < mazeSize; x++) {
		for (unsigned int y = 0; y < mazeSize; y++) {
			/*if (Utils::rnd() > 0.5f)
				continue;*/

			auto e = Entity::Create();
			e->addComponent<ModelComponent>(m_cubeModel.get());
			e->addComponent<TransformComponent>(glm::vec3(x * wallSize + mazeStart, 0.5f, y * wallSize + mazeStart));
			e->addComponent<MaterialComponent>(Material::PHONG);
			m_scene.addEntity(e);
		}
	}
}

GameState::~GameState() {
}

// Process input for the state
bool GameState::processInput(float dt) {
	SAIL_PROFILE_FUNCTION();

#ifdef _DEBUG
	// Add point light at camera pos
	if (Input::WasKeyJustPressed(SAIL_KEY_E)) {
		PointLight pl;
		pl.setColor(glm::vec3(Utils::rnd(), Utils::rnd(), Utils::rnd()));
		pl.setPosition(m_cam.getPosition());
		pl.setAttenuation(.0f, 0.1f, 0.02f);
		m_lights.addPointLight(pl);
	}

	if (Input::WasKeyJustPressed(SAIL_KEY_1)) {
		Logger::Log("Setting parent");
		m_transformTestEntities[2]->getComponent<TransformComponent>()->setParent(m_transformTestEntities[1]->getComponent<TransformComponent>());
	}
	if (Input::WasKeyJustPressed(SAIL_KEY_2)) {
		Logger::Log("Removing parent");
		m_transformTestEntities[2]->getComponent<TransformComponent>()->removeParent();
	}
#endif

	if (Input::IsKeyPressed(SAIL_KEY_G)) {
		glm::vec3 color(1.0f, 1.0f, 1.0f);;
		m_lights.setDirectionalLight(DirectionalLight(color, m_cam.getDirection()));
	}

	// Update the camera controller from input devices
	m_camController.update(dt);

	// Reload shaders
	if (Input::WasKeyJustPressed(SAIL_KEY_R)) {
		m_app->getResourceManager().reloadShader<PhongMaterialShader>();
	}

	return true;
}

bool GameState::update(float dt) {
	SAIL_PROFILE_FUNCTION();

	std::wstring fpsStr = std::to_wstring(m_app->getFPS());

	m_app->getWindow()->setWindowTitle("Sail | Game Engine Demo | " + Application::getPlatformName() + " | FPS: " + std::to_string(m_app->getFPS()));

	static float counter = 0.0f;
	static float size = 1;
	static float change = 0.4f;
	
	counter += dt * 2;
	if (m_texturedCubeEntity) {
		// Move the cubes around
		m_texturedCubeEntity->getComponent<TransformComponent>()->setTranslation(glm::vec3(glm::sin(counter), 1.f, glm::cos(counter)));
		m_texturedCubeEntity->getComponent<TransformComponent>()->setRotations(glm::vec3(glm::sin(counter), counter, glm::cos(counter)));

		// Move the three parented cubes with identical translation, rotations and scale to show how parenting affects transforms
		for (Entity::SPtr item : m_transformTestEntities) {
			item->getComponent<TransformComponent>()->rotateAroundY(dt * 1.0f);
			item->getComponent<TransformComponent>()->setScale(size);
			item->getComponent<TransformComponent>()->setTranslation(size * 3, 1.0f, size * 3);
		}
		m_transformTestEntities[0]->getComponent<TransformComponent>()->translate(2.0f, 0.0f, 2.0f);

		size += change * dt;
		if (size > 1.2f || size < 0.7f)
			change *= -1.0f;
	}

	return true;
}

// Renders the state
bool GameState::render(float dt) {
	SAIL_PROFILE_FUNCTION();

	// Clear back buffer
	m_app->getAPI()->clear({0.1f, 0.2f, 0.3f, 1.0f});

	// Draw the scene
	m_scene.draw(m_cam);

	return true;
}

bool GameState::renderImgui(float dt) {
	SAIL_PROFILE_FUNCTION();
	// The ImGui window is rendered when activated on F10
	ImGui::ShowDemoWindow();
	return false;
}
