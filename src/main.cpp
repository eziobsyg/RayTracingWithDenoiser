#include <cassert>
#include <cstring>
#define _USE_MATH_DEFINES
#include <cmath>
#include <iostream>

#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include "Camera.h"
#include "GLSL.h"
#include "MatrixStack.h"
#include "Program.h"
#include "Shape.h"
#include "Material.h"
#include "Light.h"
#include "RT_Screen.h"
#include "TimeRecord.h"
#include "Tool.h"
#include "Sphere.h"

#define MAX_LIGHTS 3
#define KEY_COUNT 349
using namespace std;

GLFWwindow *window; // Main application window
string RESOURCE_DIR = "./"; // Where the resources are loaded from
bool OFFLINE = false;

shared_ptr<Camera> camera;
shared_ptr<Program> prog;
shared_ptr<Shape> shape;
shared_ptr<Material> material;
shared_ptr<Light> light;
shared_ptr<RT_Screen> screen;
shared_ptr<RenderBuffer> screenBuffer;
shared_ptr<timeRecord> tRecord;
shared_ptr<Sphere> sphere;

bool keyToggles[KEY_COUNT] = {false}; // only for English keyboards!
char uniformName[50];

int programIndex;
int programNum;
vector<shared_ptr<Program>> programs;

int materialIndex;
int materialNum;
vector<shared_ptr<Material>> materials;

int lightIndex;
int lightNum;
vector<shared_ptr<Light>> lights;

int shapeIndex;
int shapeNum;
vector<shared_ptr<Shape>> shapes;

int sppIndex;
int sppNum;
vector<shared_ptr<int>> spps;

int sphereIndex;
int sphereNum;
vector<shared_ptr<Sphere>> spheres;

unsigned int SCR_WIDTH = 1200;
unsigned int SCR_HEIGHT = 800;

bool temporalDenoiser = false;
bool spatialDenoiser = false;

float globalLight;

// This function is called when a GLFW error occurs
static void error_callback(int error, const char *description)
{
	cerr << description << endl;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);

// https://lencerf.github.io/post/2019-09-21-save-the-opengl-rendering-to-image-file/
static void saveImage(const char *filepath, GLFWwindow *w)
{
	int width, height;
	glfwGetFramebufferSize(w, &width, &height);
	GLsizei nrChannels = 3;
	GLsizei stride = nrChannels * width;
	stride += (stride % 4) ? (4 - stride % 4) : 0;
	GLsizei bufferSize = stride * height;
	std::vector<char> buffer(bufferSize);
	glPixelStorei(GL_PACK_ALIGNMENT, 4);
	glReadBuffer(GL_BACK);
	glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, buffer.data());
	stbi_flip_vertically_on_write(true);
	int rc = stbi_write_png(filepath, width, height, nrChannels, buffer.data(), stride);
	if(rc) {
		cout << "Wrote to " << filepath << endl;
	} else {
		cout << "Couldn't write to " << filepath << endl;
	}
}

// This function is called once to initialize the scene and OpenGL
static void init()
{
	// Initial programs
	programNum = 2;
	for (int i = 0; i < programNum; ++i) {
		programs.push_back(make_shared<Program>());
	}

	prog = programs[0];
	prog->setShaderNames(RESOURCE_DIR + "RayTracerVertexShader.glsl", RESOURCE_DIR + "RayTracerFragmentShader.glsl");
	prog->setVerbose(true);
	prog->init();
	prog->addUniform("historyTexture");
	prog->addUniform("historyDepthTexture");
	prog->addUniform("historyNormalTexture");
	prog->addUniform("historyCountTexture");
	prog->addUniform("historyluminance1Texture");
	prog->addUniform("historyluminance2Texture");
	prog->addUniform("temporalDenoiser");
	prog->addUniform("spatialDenoiser");
	prog->addUniform("sphereNum");
	prog->addUniform("globalLight");
	GLSL::checkError(GET_FILE_LINE);
	//camera
	prog->addUniform("camera.camPos");
	prog->addUniform("camera.front");
	prog->addUniform("camera.right");
	prog->addUniform("camera.up");
	prog->addUniform("camera.halfH");
	prog->addUniform("camera.halfW");
	prog->addUniform("camera.leftbottom");
	prog->addUniform("camera.LoopNum");
	//random
	prog->addUniform("randOrigin");

	//sphere
	sphereNum = 8;
	for (int i = 0; i < sphereNum; ++i) {
		sprintf(uniformName, "sphere[%d].radius", i);
		prog->addUniform(string(uniformName));

		sprintf(uniformName, "sphere[%d].center", i);
		prog->addUniform(string(uniformName));

		sprintf(uniformName, "sphere[%d].materialIndex", i);
		prog->addUniform(string(uniformName));

		sprintf(uniformName, "sphere[%d].albedo", i);
		prog->addUniform(string(uniformName));
	}


	prog->addUniform("spp");
	prog->setVerbose(false);
	
	prog = programs[1];
	prog->setShaderNames(RESOURCE_DIR + "ScreenVertexShader.glsl", RESOURCE_DIR + "ScreenFragmentShader.glsl");
	prog->setVerbose(true);
	prog->init();
	prog->addUniform("spatialDenoiser");
	prog->addUniform("screenTexture");
	prog->addUniform("historyluminance1Texture");
	prog->addUniform("historyluminance2Texture");
	prog->addUniform("texelWidth");
	prog->addUniform("texelHeight");
	prog->setVerbose(false);
	// Initial materials
	materialIndex = 0;
	materialNum = 3;
	for (int i = 0; i < materialNum; ++i) {
		materials.push_back(make_shared<Material>());
	}

	material = materials[0];
	material->ka = glm::vec3(0.2f, 0.2f, 0.2f);
	material->kd = glm::vec3(0.8f, 0.7f, 0.7f);
	material->ks = glm::vec3(1.0f, 0.9f, 0.8f);
	material->s = 200.0f;

	material = materials[1];
	material->ka = glm::vec3(0.2f, 0.2f, 0.2f);
	material->kd = glm::vec3(0.0f, 0.0f, 1.0f);
	material->ks = glm::vec3(0.0f, 1.0f, 0.0f);
	material->s = 100.0f;

	material = materials[2];
	material->ka = glm::vec3(0.2f, 0.2f, 0.2f);
	material->kd = glm::vec3(0.3f, 0.3f, 0.3f);
	material->ks = glm::vec3(0.1f, 0.1f, 0.1f);
	material->s = 100.0f;

	// Initial lights
	lightNum = 2;
	for (int i = 0; i < lightNum; ++i) {
		lights.push_back(make_shared<Light>());
	}

	light = lights[0];
	light->position = glm::vec3(1.0, 1.0, 1.0);
	light->color = glm::vec3(0.8, 0.8, 0.8);

	light = lights[1];
	light->position = glm::vec3(-1.0, 1.0, 1.0);
	light->color = glm::vec3(0.2, 0.2, 0.0);

	// Initial shapes
	/*shapeNum = 2;
	for (int i = 0; i < shapeNum; ++i) {
		shapes.push_back(make_shared<Shape>());
	}
	shape = shapes[0];
	shape->loadMesh(RESOURCE_DIR + "bunny.obj");
	shape->init();
	shape = shapes[1];
	shape->loadMesh(RESOURCE_DIR + "teapot.obj");
	shape->init();*/

	// Initial sphere
	sphereIndex = 0;
	for (int i = 0; i < sphereNum; ++i) {
		spheres.push_back(make_shared<Sphere>());
	}

	sphere = spheres[0];
	sphere->center = glm::vec3(0.0, -100.5, -1.0);
	sphere->radius = 100.0;
	sphere->materialIndex = 1;
	sphere->albedo = glm::vec3(0.5, 0.5, 0.5);

	sphere = spheres[1];
	sphere->center = glm::vec3(1.0, 0.0, -1.0);
	sphere->radius = 0.5;
	sphere->materialIndex = 3;
	sphere->albedo = glm::vec3(0.2, 0.7, 0.6);

	sphere = spheres[2];
	sphere->center = glm::vec3(-1.0, 0.0, -1.0);
	sphere->radius = 0.5;
	sphere->materialIndex = 2;
	sphere->albedo = glm::vec3(0.1, 0.3, 0.7);

	sphere = spheres[3];
	sphere->center = glm::vec3(-2.0, 0.0, -1.0);
	sphere->radius = 0.5;
	sphere->materialIndex = 1;
	sphere->albedo = glm::vec3(0.1, 0.8, 0.3);

	sphere = spheres[4];
	sphere->center = glm::vec3(0.0, 1.0, -5.0);
	sphere->radius = 2;
	sphere->materialIndex = 3;
	sphere->albedo = glm::vec3(1.0, 1.0, 1.0);

	sphere = spheres[5];
	sphere->center = glm::vec3(0.0, 0.0, -1.0);
	sphere->radius = 0.5;
	sphere->materialIndex = 3;
	sphere->albedo = glm::vec3(1.0, 1.0, 1.0);

	sphere = spheres[6];
	sphere->center = glm::vec3(0.0, 3.0, 0.0);
	sphere->radius = 1.0;
	sphere->materialIndex = 0;
	sphere->albedo = glm::vec3(1.0, 1.0, 1.0);

	sphere = spheres[7];
	sphere->center = glm::vec3(3.0, 3.0, 0.0);
	sphere->radius = 1.0;
	sphere->materialIndex = 0;
	sphere->albedo = glm::vec3(1.0, 1.0, 1.0);

	// Initial spp
	sppNum = 5;
	spps.push_back(make_shared<int>(1));
	spps.push_back(make_shared<int>(3));
	spps.push_back(make_shared<int>(5));
	spps.push_back(make_shared<int>(10));
	spps.push_back(make_shared<int>(20));
	sppIndex = 0;

	// Initial screen
	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	camera = make_shared<Camera>(width, height);
	
	screen = make_shared<RT_Screen>();
	screen->InitScreenBind();

	screenBuffer = make_shared<RenderBuffer>();
	screenBuffer->Init(width, height);
	CPURandomInit();

	tRecord = make_shared<timeRecord>();

	globalLight = 1.0;

	GLSL::checkError(GET_FILE_LINE);
}

// This function is called every frame to draw the scene.
static void render()
{
	// compute time
	tRecord->updateTime();

	// input by keyboard
	processInput(window);

	// camera loop add 1
	camera->LoopIncrease();

	screenBuffer->setCurrentBuffer(camera->LoopNum);

	prog = programs[0];
	prog->bind();
	glUniform1i(prog->getUniform("historyTexture"), 0);
	glUniform1i(prog->getUniform("historyDepthTexture"), 1);
	glUniform1i(prog->getUniform("historyNormalTexture"), 2);
	glUniform1i(prog->getUniform("historyCountTexture"), 3);
	glUniform1i(prog->getUniform("historyluminance1Texture"), 4);
	glUniform1i(prog->getUniform("historyluminance2Texture"), 5);
	glUniform1i(prog->getUniform("temporalDenoiser"), temporalDenoiser);
	glUniform1i(prog->getUniform("spatialDenoiser"), spatialDenoiser);
	glUniform1i(prog->getUniform("sphereNum"), sphereNum);
	glUniform1f(prog->getUniform("globalLight"), globalLight);
	//camera
	glUniform3fv(prog->getUniform("camera.camPos"), 1, &camera->cameraPos[0]);
	glUniform3fv(prog->getUniform("camera.front"), 1, &camera->cameraFront[0]);
	glUniform3fv(prog->getUniform("camera.right"), 1, &camera->cameraRight[0]);
	glUniform3fv(prog->getUniform("camera.up"), 1, &camera->cameraUp[0]);
	glUniform1f(prog->getUniform("camera.halfH"), camera->halfH);
	glUniform1f(prog->getUniform("camera.halfW"), camera->halfW);
	glUniform3fv(prog->getUniform("camera.leftbottom"), 1, &camera->LeftBottomCorner[0]);
	glUniform1i(prog->getUniform("camera.LoopNum"), camera->LoopNum);

	//random
	glUniform1f(prog->getUniform("randOrigin"), 674764.0f * (GetCPURandom() + 1.0f));
	glUniform1i(prog->getUniform("spp"), *spps[sppIndex]);

	//sphere
	for (int i = 0; i < sphereNum; ++i) {
		sprintf(uniformName, "sphere[%d].radius", i);
		glUniform1f(prog->getUniform(uniformName), spheres[i]->radius);

		sprintf(uniformName, "sphere[%d].center", i);
		glUniform3fv(prog->getUniform(uniformName), 1, &spheres[i]->center[0]);

		sprintf(uniformName, "sphere[%d].materialIndex", i);
		glUniform1i(prog->getUniform(uniformName), spheres[i]->materialIndex);

		sprintf(uniformName, "sphere[%d].albedo", i);
		glUniform3fv(prog->getUniform(uniformName), 1, &spheres[i]->albedo[0]);
	}

	screen->DrawScreen();
	prog->unbind();

	int width, height;
	glfwGetFramebufferSize(window, &width, &height);

	prog = programs[1];
	// 绑定到默认缓冲区
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	// 清屏
	glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	prog->bind();
	screenBuffer->setCurrentAsTexture(camera->LoopNum);
	// screenBuffer绑定的纹理被定义为纹理0，所以这里设置片段着色器中的screenTexture为纹理0
	glUniform1i(prog->getUniform("spatialDenoiser"), spatialDenoiser);
	glUniform1i(prog->getUniform("screenTexture"), 0);
	glUniform1i(prog->getUniform("historyluminance1Texture"), 4);
	glUniform1i(prog->getUniform("historyluminance2Texture"), 5);
	glUniform1f(prog->getUniform("texelWidth"), 1.0f / width);
	glUniform1f(prog->getUniform("texelHeight"), 1.0f / height);

	// 绘制屏幕
	screen->DrawScreen();
	
	GLSL::checkError(GET_FILE_LINE);
	
	if(OFFLINE) {
		saveImage("output.png", window);
		GLSL::checkError(GET_FILE_LINE);
		glfwSetWindowShouldClose(window, true);
	}
}

int main(int argc, char **argv)
{
	if(argc < 2) {
		cout << "Usage: A3 RESOURCE_DIR" << endl;
		return 0;
	}
	RESOURCE_DIR = argv[1] + string("/");
	
	// Optional argument
	if(argc >= 3) {
		OFFLINE = atoi(argv[2]) != 0;
	}

	// Set error callback.
	glfwSetErrorCallback(error_callback);
	// Initialize the library.
	if(!glfwInit()) {
		return -1;
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// Create a windowed mode window and its OpenGL context.
	window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "YOUR NAME", NULL, NULL);
	if(!window) {
		glfwTerminate();
		return -1;
	}
	// Make the window's context current.
	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetScrollCallback(window, scroll_callback);

	// 窗口捕获鼠标，不显示鼠标
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	// Initialize GLEW.
	glewExperimental = true;
	if(glewInit() != GLEW_OK) {
		cerr << "Failed to initialize GLEW" << endl;
		return -1;
	}
	glGetError(); // A bug in glewInit() causes an error that we can safely ignore.
	cout << "OpenGL version: " << glGetString(GL_VERSION) << endl;
	cout << "GLSL version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;
	GLSL::checkVersion();
	// Set vsync.
	glfwSwapInterval(1);

	// Initialize scene.
	init();
	// Loop until the user closes the window.
	while(!glfwWindowShouldClose(window)) {
		// Render scene.
		render();
		// Swap front and back buffers.
		glfwSwapBuffers(window);
		// Poll for and process events.
		glfwPollEvents();
	}
	// Quit program.
	glfwDestroyWindow(window);
	glfwTerminate();
	// delete resources
	screenBuffer->Delete();
	screen->Delete();

	return 0;
}

// 按键处理
// WSAD move camera
// XYZ shift move object
// <> chose object
// up chose spp
// O enable/disable temporal denoiser
// p enable/disable spatial denoiser
// + increase global light
void processInput(GLFWwindow* window) {
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		camera->ProcessKeyboard(FORWARD, tRecord->deltaTime);
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		camera->ProcessKeyboard(BACKWARD, tRecord->deltaTime);
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		camera->ProcessKeyboard(LEFT, tRecord->deltaTime);
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		camera->ProcessKeyboard(RIGHT, tRecord->deltaTime);
	if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS) {
		if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS) {
			spheres[sphereIndex]->ProcessKeyboard(x, tRecord->deltaTime);
		}
		else {
			spheres[sphereIndex]->ProcessKeyboard(X, tRecord->deltaTime);
		}
	}
	if (glfwGetKey(window, GLFW_KEY_Y) == GLFW_PRESS) {
		if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS) {
			spheres[sphereIndex]->ProcessKeyboard(y, tRecord->deltaTime);
		}
		else {
			spheres[sphereIndex]->ProcessKeyboard(Y, tRecord->deltaTime);
		}
	}
	if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS) {
		if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS) {
			spheres[sphereIndex]->ProcessKeyboard(z, tRecord->deltaTime);
		}
		else {
			spheres[sphereIndex]->ProcessKeyboard(Z, tRecord->deltaTime);
		}
	}
	if (glfwGetKey(window, GLFW_KEY_EQUAL) == GLFW_PRESS) {
		if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS) {
			globalLight -= 0.01;
			if (globalLight < 0) {
				globalLight = 0;
			}
		}
		else {
			globalLight += 0.01;
			if (globalLight > 1) {
				globalLight = 1;
			}
		}
	}

	if (glfwGetKey(window, GLFW_KEY_PERIOD) == GLFW_PRESS) {
		if (!keyToggles[GLFW_KEY_PERIOD]) {
			keyToggles[GLFW_KEY_PERIOD] = true;
			sphereIndex = (sphereIndex + 1) % sphereNum;
			cout << "sphere: " << sphereIndex << endl;
		}
	}
	else {
		keyToggles[GLFW_KEY_PERIOD] = false;
	}

	if (glfwGetKey(window, GLFW_KEY_COMMA) == GLFW_PRESS) {
		if (!keyToggles[GLFW_KEY_COMMA]) {
			keyToggles[GLFW_KEY_COMMA] = true;
			sphereIndex = (sphereIndex + sphereNum - 1) % sphereNum;
			cout << "sphere: " << sphereIndex << endl;
		}
	}
	else {
		keyToggles[GLFW_KEY_COMMA] = false;
	}

	if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
		if (!keyToggles[GLFW_KEY_UP]) {
			keyToggles[GLFW_KEY_UP] = true;
			sppIndex = (sppIndex + 1) % sppNum;
			cout << "ssp: " << *spps[sppIndex] << endl;
		}
	}
	else {
		keyToggles[GLFW_KEY_UP] = false;
	}

	if (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS) {
		if (!keyToggles[GLFW_KEY_O]) {
			keyToggles[GLFW_KEY_O] = true;
			temporalDenoiser = 1 - temporalDenoiser;
			if (temporalDenoiser) {
				cout << "Enable temporalDenoiser" << endl;
			}
			else {
				cout << "Disable temporalDenoiser" << endl;
			}
		}
	}
	else {
		keyToggles[GLFW_KEY_O] = false;
	}

	if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS) {
		if (!keyToggles[GLFW_KEY_P]) {
			keyToggles[GLFW_KEY_P] = true;
			spatialDenoiser = 1 - spatialDenoiser;
			if (spatialDenoiser) {
				cout << "Enable spatialDenoiser" << endl;
			}
			else {
				cout << "Disable spatialDenoiser" << endl;
			}
		}
	}
	else {
		keyToggles[GLFW_KEY_P] = false;
	}
}

// 处理窗口尺寸变化
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
	camera->updateScreenRatio(width, height);
	glViewport(0, 0, width, height);
}

// 鼠标事件响应
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn) {
	float xpos = static_cast<float>(xposIn);
	float ypos = static_cast<float>(yposIn);
	camera->updateCameraFront(xpos, ypos);
}

// 设置fov
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
	camera->updateFov(static_cast<float>(yoffset));
}
