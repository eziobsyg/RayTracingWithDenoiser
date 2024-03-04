#pragma once
#ifndef MATERIAL_H
#define MATERIAL_H

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

class Material
{
public:
	Material() = default;
	Material(const glm::vec3 & inputKa, const glm::vec3 & inputKd, const glm::vec3 & inputKs, float inputS) :
		ka(inputKa), kd(inputKd), ks(inputKs), s(inputS) {}

	glm::vec3 ka = glm::vec3(1.0f);
	glm::vec3 kd = glm::vec3(1.0f);
	glm::vec3 ks = glm::vec3(1.0f);
	float s = 0.0f;
};

#endif