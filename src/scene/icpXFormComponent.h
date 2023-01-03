#pragma once

#include "../core/icpMacros.h"
#include "../core/icpGuid.h"
#include "icpComponent.h"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "icpEntity.h"

INCEPTION_BEGIN_NAMESPACE
	class icpXFormComponent : public icpComponentBase
{
public:
	icpXFormComponent() = default;
	virtual ~icpXFormComponent() = default;

	std::shared_ptr<icpXFormComponent> m_parent = nullptr;

	std::vector<std::shared_ptr<icpXFormComponent>> m_children;

	std::weak_ptr<icpGameEntity> m_entity;

	glm::vec3 m_translation{ 0.f, 0.f, 0.f };
	glm::vec3 m_rotation{ 0.f, 0.f, 0.f };
	glm::qua<float> m_quternionRot {1.f, 0.f, 0.f, 0.f};

	glm::vec3 m_scale{ 1.f,1.f,1.f };
};


INCEPTION_END_NAMESPACE