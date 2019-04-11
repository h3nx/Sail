#include "Object.h"

Object::Object()
	: model(nullptr)
	, lightColor(Vector4(1.f))
{
	boundingBox = nullptr;
}

Object::~Object() {
	Memory::safeDelete(this->boundingBox);
}

void Object::setPosition(const glm:::vec3 &newPosition) {
	m_transform.setTranslation(newPosition);
	updateBoundingBox();
}

void Object::updateBoundingBox(bool includeBoundingBoxRotation)
{
	if (this->boundingBox) {
		if (includeBoundingBoxRotation)
			this->boundingBox->updateTransform(this->getTransform().getMatrix());
			//this->boundingBox->updateTranslation(this->getTransform().getTranslation());
		else
			this->boundingBox->updateTranslation(this->getTransform().getTranslation());
	}
}

void Object::setModel(Model* model)
{
	if (model) {
		this->model = model;
		if (&model->getAABB()) {
			if (this->boundingBox) {
				delete this->boundingBox;
			}
			this->boundingBox = new AABB(model->getAABB());
		}
		else {
			
			Logger::Error("BoundingBox not loaded");
		}
	}
	else {
		this->model = nullptr;
		Memory::safeDelete(this->boundingBox);
	}
}

void Object::setLightColor(glm:::vec4 color)
{
	this->lightColor = color;
}

Model* Object::getModel()
{
	return this->model;
}

Transform& Object::getTransform() {
	return m_transform;
}

AABB* Object::getBoundingBox() {
	return boundingBox;
}

glm:::vec4 Object::getLightColor() {
	return lightColor;
}

glm:::vec4 Object::getColor() {
	return model->getMaterial()->getColor();
}