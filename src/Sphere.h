#pragma once
#ifndef SPHERE_H
#define SPHERE_H

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

enum Sphere_Movement {
	X,
	x,
	Y,
	y,
	Z,
	z
};

class Sphere
{
public:
	Sphere() = default;
	Sphere(const glm::vec3& Pos, float r, int i, const glm::vec3& a) :
		center(Pos), radius(r), materialIndex(i), albedo(a) {}
	void ProcessKeyboard(Sphere_Movement direction, float deltaTime) {
		float velocity = sphereSpeed * deltaTime;
		if (direction == X)
			center.x +=  velocity;
		if (direction == x)
			center.x -= velocity;
		if (direction == Y)
			center.y += velocity;
		if (direction == y)
			center.y -= velocity;
		if (direction == Z)
			center.z += velocity;
		if (direction == z)
			center.z -= velocity;
	}

	glm::vec3 center = glm::vec3(1.0f);
	float radius = 1.0f;
	int materialIndex = 0;
	glm::vec3 albedo = glm::vec3(1.0f);
	float sphereSpeed = 1.0f;
};

#endif