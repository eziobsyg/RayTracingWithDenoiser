#pragma once
#ifndef TIMERECORD_h
#define TIMERECORD_h

#include <GLFW/glfw3.h>

class timeRecord {
public:
	timeRecord() {
		lastFrameTime = 0.0f;
	}

	void updateTime() {
		currentFrameTime = static_cast<float>(glfwGetTime());
		deltaTime = currentFrameTime - lastFrameTime;
		lastFrameTime = currentFrameTime;
	}

	float currentFrameTime;
	float lastFrameTime;
	float deltaTime;

};
#endif