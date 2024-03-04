#pragma once
#ifndef LIGHT_H
#define LIGHT_H

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

class Light
{
public:
	Light() = default;
	Light(const glm::vec3& inputPos, const glm::vec3& inputColor) :
		position(inputPos), color(inputColor) {}

	glm::vec3 position = glm::vec3(1.0f);
	glm::vec3 color = glm::vec3(1.0f);
};

#endif