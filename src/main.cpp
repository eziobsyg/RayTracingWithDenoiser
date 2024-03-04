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

unsigned int SCR_WIDTH = 1200;
unsigned int SCR_HEIGHT = 800;

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
	prog->addUniform("sphere[0].radius");
	prog->addUniform("sphere[0].center");
	prog->addUniform("sphere[0].materialIndex");
	prog->addUniform("sphere[0].albedo");

	prog->addUniform("sphere[1].radius");
	prog->addUniform("sphere[1].center");
	prog->addUniform("sphere[1].materialIndex");
	prog->addUniform("sphere[1].albedo");

	prog->addUniform("sphere[2].radius");
	prog->addUniform("sphere[2].center");
	prog->addUniform("sphere[2].materialIndex");
	prog->addUniform("sphere[2].albedo");

	prog->addUniform("sphere[3].radius");
	prog->addUniform("sphere[3].center");
	prog->addUniform("sphere[3].materialIndex");
	prog->addUniform("sphere[3].albedo");

	prog->addUniform("sphere[4].radius");
	prog->addUniform("sphere[4].center");
	prog->addUniform("sphere[4].materialIndex");
	prog->addUniform("sphere[4].albedo");

	prog->addUniform("sphere[5].radius");
	prog->addUniform("sphere[5].center");
	prog->addUniform("sphere[5].materialIndex");
	prog->addUniform("sphere[5].albedo");

	prog->addUniform("spp");
	prog->setVerbose(false);
	
	prog = programs[1];
	prog->setShaderNames(RESOURCE_DIR + "ScreenVertexShader.glsl", RESOURCE_DIR + "ScreenFragmentShader.glsl");
	prog->setVerbose(true);
	prog->init();
	prog->addUniform("historyTexture");
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
	glm::vec3 center;
	glm::vec3 albedo;
	glUniform1f(prog->getUniform("sphere[0].radius"), 0.5);
	center = glm::vec3(0.0, 0.0, -1.0);
	glUniform3fv(prog->getUniform("sphere[0].center"), 1, &center[0]);
	glUniform1i(prog->getUniform("sphere[0].materialIndex"), 2);
	albedo = glm::vec3(1.0, 1.0, 1.0);
	glUniform3fv(prog->getUniform("sphere[0].albedo"), 1, &albedo[0]);

	glUniform1f(prog->getUniform("sphere[1].radius"), 0.5);
	center = glm::vec3(1.0, 0.0, -1.0);
	glUniform3fv(prog->getUniform("sphere[1].center"), 1, &center[0]);
	glUniform1i(prog->getUniform("sphere[1].materialIndex"), 2);
	albedo = glm::vec3(0.2, 0.7, 0.6);
	glUniform3fv(prog->getUniform("sphere[1].albedo"), 1, &albedo[0]);

	glUniform1f(prog->getUniform("sphere[2].radius"), 0.5);
	center = glm::vec3(-1.0, 0.0, -1.0);
	glUniform3fv(prog->getUniform("sphere[2].center"), 1, &center[0]);
	glUniform1i(prog->getUniform("sphere[2].materialIndex"), 1);
	albedo = glm::vec3(0.1, 0.3, 0.7);
	glUniform3fv(prog->getUniform("sphere[2].albedo"), 1, &albedo[0]);

	glUniform1f(prog->getUniform("sphere[3].radius"), 0.5);
	center = glm::vec3(-2.0, 0.0, -1.0);
	glUniform3fv(prog->getUniform("sphere[3].center"), 1, &center[0]);
	glUniform1i(prog->getUniform("sphere[3].materialIndex"), 0);
	albedo = glm::vec3(0.1, 0.8, 0.3);
	glUniform3fv(prog->getUniform("sphere[3].albedo"), 1, &albedo[0]);

	glUniform1f(prog->getUniform("sphere[4].radius"), 2);
	center = glm::vec3(0.0, 1.0, -5.0);
	glUniform3fv(prog->getUniform("sphere[4].center"), 1, &center[0]);
	glUniform1i(prog->getUniform("sphere[4].materialIndex"), 2);
	albedo = glm::vec3(1.0, 1.0, 1.0);
	glUniform3fv(prog->getUniform("sphere[4].albedo"), 1, &albedo[0]);

	glUniform1f(prog->getUniform("sphere[5].radius"), 100.0);
	center = glm::vec3(0.0, -100.5, -1.0);
	glUniform3fv(prog->getUniform("sphere[5].center"), 1, &center[0]);
	glUniform1i(prog->getUniform("sphere[5].materialIndex"), 0);
	albedo = glm::vec3(0.9, 0.9, 0.9);
	glUniform3fv(prog->getUniform("sphere[5].albedo"), 1, &albedo[0]);

	screen->DrawScreen();
	prog->unbind();

	prog = programs[1];
	// 绑定到默认缓冲区
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	// 清屏
	glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	prog->bind();
	screenBuffer->setCurrentAsTexture(camera->LoopNum);
	// screenBuffer绑定的纹理被定义为纹理0，所以这里设置片段着色器中的screenTexture为纹理0
	glUniform1i(prog->getUniform("screenTexture"), 0);

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
	if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
		if (!keyToggles[GLFW_KEY_UP]) {
			keyToggles[GLFW_KEY_UP] = true;
			sppIndex = (sppIndex + 1) % sppNum;
			cout << *spps[sppIndex] << endl;
		}
	}
	else {
		keyToggles[GLFW_KEY_UP] = false;
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
