#include "ModelViewerGui.h"
#include "Sail.h"
#include <imgui/imgui.cpp>
#include <commdlg.h>
#include <atlstr.h>
#include <glm/gtc/type_ptr.hpp>

ModelViewerGui::ModelViewerGui() {
	m_modelName = "None - click to load";
}

void ModelViewerGui::render(float dt, FUNC(void()) funcSwitchState, FUNC(void(const std::string&)) callbackNewModel, Entity* entity) {
	m_funcSwitchState = funcSwitchState;
	m_callbackNewModel = callbackNewModel;

	float menuBarHeight = setupMenuBar();

	// Dockspace
	ImGuiIO& io = ImGui::GetIO();
	if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable) {
		setupDockspace(menuBarHeight);
	}

	// Model customization window
	ImGui::Begin("Model customizer");

	float propWidth = 0;
	int propID = 0;
	auto addProperty = [&propWidth, &propID](const char* label, std::function<void()> prop, bool setWidth = true) {
		ImGui::PushID(propID++);
		ImGui::AlignTextToFramePadding();
		ImGui::Text(label);
		ImGui::NextColumn();
		if (setWidth)
			ImGui::SetNextItemWidth(propWidth);
		prop();
		ImGui::NextColumn();
		ImGui::PopID();
	};
	auto disableColumns = []() {
		ImGui::Columns(1);
	};
	auto enableColumns = [&propWidth, &propID](float labelWidth = 75.f) {
		ImGui::PushID(propID++);
		ImGui::Columns(2, "alignedList", false);  // 3-ways, no border
		ImGui::SetColumnWidth(0, labelWidth);
		propWidth = ImGui::GetColumnWidth(1) - 10;
		ImGui::PopID();
	};

	enableColumns();

	limitStringLength(m_modelName);
	addProperty("Model", [&]() {
		if (ImGui::Button(m_modelName.c_str(), ImVec2(propWidth, 0))) {
			std::string newModel = openFilename(L"FBX models (*.fbx)\0*.fbx");
			if (!newModel.empty()) {
				m_modelName = newModel;
				m_callbackNewModel(m_modelName);
			}
		}
	});
		
	static const char* lbl = "##hidelabel";
	
	// ================================
	//			 MATERIAL GUI
	// ================================
	PhongMaterial* material = nullptr;
	ModelComponent* model = entity->getComponent<ModelComponent>();
	if (model) {
		material = model->getModel()->getMesh(0)->getMaterial();
	}
	if (material) {
		disableColumns();
		ImGui::Separator();
		ImGui::Text("Shader pipeline: %s", material->getShader()->getPipeline()->getName().c_str()); ImGui::NextColumn();
		ImGui::Separator();
		enableColumns();

		addProperty("Ambient", [&] { ImGui::SliderFloat(lbl, &material->getPhongSettings().ka, 0.f, 10.f); });
		addProperty("Diffuse", [&] { ImGui::SliderFloat(lbl, &material->getPhongSettings().kd, 0.f, 10.f); });
		addProperty("Specular", [&] { ImGui::SliderFloat(lbl, &material->getPhongSettings().ks, 0.f, 10.f); });

		addProperty("Shininess", [&] { ImGui::SliderFloat(lbl, &material->getPhongSettings().shininess, 0.f, 100.f); });

		addProperty("Color", [&] { ImGui::ColorEdit4(lbl, glm::value_ptr(material->getPhongSettings().modelColor)); });

		disableColumns();
		ImGui::Separator();
		ImGui::Text("Textures");
		enableColumns();

		float trashButtonWidth = 21.f;
		std::string diffuseTexName = (material->getTexture(0)) ? material->getTexture(0)->getName() : "None - click to load";
		limitStringLength(diffuseTexName);
		addProperty("Diffuse", [&]() {
			if (ImGui::Button(diffuseTexName.c_str(), ImVec2(propWidth - trashButtonWidth - ImGui::GetStyle().ItemSpacing.x, 0))) {
				std::string filename = openFilename(L"TGA textures (*.tga)\0*.tga");
				if (!filename.empty()) {
					material->setDiffuseTexture(filename, true);
				}
			}
			ImGui::SameLine();
			if (ImGui::Button(ICON_FA_TRASH, ImVec2(trashButtonWidth, 0))) {
				material->setDiffuseTexture("");
			}
		}, false);

		std::string normalTexName = (material->getTexture(1)) ? material->getTexture(1)->getName() : "None - click to load";
		limitStringLength(normalTexName);
		addProperty("Normal", [&]() {
			if (ImGui::Button(normalTexName.c_str(), ImVec2(propWidth - trashButtonWidth - ImGui::GetStyle().ItemSpacing.x, 0))) {
				std::string filename = openFilename(L"TGA textures (*.tga)\0*.tga");
				if (!filename.empty()) {
					material->setNormalTexture(filename, true);
				}
			}
			ImGui::SameLine();
			if (ImGui::Button(ICON_FA_TRASH, ImVec2(trashButtonWidth, 0))) {
				material->setNormalTexture("");
			}
		}, false);

		std::string specularTexName = (material->getTexture(2)) ? material->getTexture(2)->getName() : "None - click to load";
		limitStringLength(specularTexName);
		addProperty("Specular", [&]() {
			if (ImGui::Button(specularTexName.c_str(), ImVec2(propWidth - trashButtonWidth - ImGui::GetStyle().ItemSpacing.x, 0))) {
				std::string filename = openFilename(L"TGA textures (*.tga)\0*.tga");
				if (!filename.empty()) {
					material->setSpecularTexture(filename, true);
				}
			}
			ImGui::SameLine();
			if (ImGui::Button(ICON_FA_TRASH, ImVec2(trashButtonWidth, 0))) {
				material->setSpecularTexture("");
			}
		}, false);
	}

	// ================================
	//			TRANSFORM GUI
	// ================================
	TransformComponent* transform = entity->getComponent<TransformComponent>();
	if (model && transform) {
		disableColumns();
		ImGui::Separator();
		ImGui::Text("Transform");
		enableColumns(90.f);

		addProperty("Translation", [&] {
			static float translation[3];
			memcpy(translation, glm::value_ptr(transform->getTranslation()), sizeof(translation));
			if (ImGui::DragFloat3(lbl, translation, 0.05f)) {
				transform->setTranslation(glm::make_vec3(translation));
			}
		});
		addProperty("Rotation", [&] {
			static float rotation[3];
			memcpy(rotation, glm::value_ptr(transform->getRotations()), sizeof(rotation));
			if (ImGui::DragFloat3(lbl, rotation, 0.01f)) {
				transform->setRotations(glm::make_vec3(rotation));
			}
		});
		addProperty("Scale", [&] {
			static float scale[3];
			memcpy(scale, glm::value_ptr(transform->getScale()), sizeof(scale));
			if (ImGui::DragFloat3(lbl, scale, 0.05f)) {
				transform->setScale(glm::make_vec3(scale));
			}
		});
		
	}

	disableColumns();

	ImGui::End();

}

void ModelViewerGui::setupDockspace(float menuBarHeight) {
	ImGuiViewport* viewport = ImGui::GetMainViewport();

	ImVec2 windowPos = viewport->Pos;
	windowPos.y += menuBarHeight;
	ImGui::SetNextWindowPos(windowPos);
	ImGui::SetNextWindowSize(viewport->Size);
	ImGui::SetNextWindowViewport(viewport->ID);

	ImGuiWindowFlags host_window_flags = 0;
	host_window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDocking;
	host_window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
	host_window_flags |= ImGuiWindowFlags_NoBackground;

	char label[32];
	ImFormatString(label, IM_ARRAYSIZE(label), "DockSpaceViewport_%08X", viewport->ID);

	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	ImGui::Begin(label, NULL, host_window_flags);
	ImGui::PopStyleVar(3);
	
	ImGuiID dockspace_id = ImGui::GetID("DockSpace");
	ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);

	ImGui::End();
}

float ModelViewerGui::setupMenuBar(){
	static bool showDemoWindow;
	ImVec2 menuBarSize;
	if (ImGui::BeginMainMenuBar()) {
		if (ImGui::BeginMenu("Duffle")) {
			ImGui::MenuItem("Show demo window", NULL, &showDemoWindow);
			if (ImGui::MenuItem("Switch to GameState", NULL)) { m_funcSwitchState(); }
			if (ImGui::MenuItem("Exit", NULL)) { PostQuitMessage(0); }
			ImGui::EndMenu();
		}

		menuBarSize = ImGui::GetWindowSize();
		ImGui::EndMainMenuBar();
	}
	if (showDemoWindow) {
		ImGui::ShowDemoWindow();
	}
	return menuBarSize.y;
}

std::string ModelViewerGui::openFilename(LPCWSTR filter, HWND owner) {
	OPENFILENAME ofn;
	WCHAR fileName[MAX_PATH] = L"";
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = owner;
	ofn.lpstrFilter = filter;
	ofn.lpstrFile = fileName;
	ofn.nMaxFile = MAX_PATH;
	ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_NOCHANGEDIR;
	ofn.lpstrDefExt = L"";
	std::string fileNameStr;
	if (GetOpenFileName(&ofn))
		fileNameStr = CW2A(fileName);
	return fileNameStr;
}

void ModelViewerGui::limitStringLength(std::string& str, int maxLength) {
	if (str.length() > maxLength) {
		str = "..." + str.substr(str.length() - maxLength + 3, maxLength - 3);
	}
}
