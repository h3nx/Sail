#pragma once

#include "Renderer.h"

class ForwardRenderer : public Renderer {
public:
	ForwardRenderer();
	~ForwardRenderer();

	void begin(Camera* camera) override;
	void submit(Model* model, const DirectX::SimpleMath::Matrix& modelMatrix);
	void submit(Mesh* mesh, const DirectX::SimpleMath::Matrix& modelMatrix) override;
	void setLightSetup(LightSetup* lightSetup) override;
	void end() override;
	void present(RenderableTexture* output = nullptr) override;

private:
	Camera* m_camera;
	LightSetup* m_lightSetup;

};