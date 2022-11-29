/*
HOMEWORK: PHYSICS ENGINE PHASE 2
MEMBERS : Junma Ogawa
Julian Urrutia
*/

//-------------------- INCLUDES --------------------\\

#include <stdio.h>
#include <stdlib.h>

#include <iostream>
#include <vector>
#include <algorithm>

#include <crtdbg.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
//#include "glm/glm.hpp"
#include "glm/ext.hpp"

#define TINYOBJLOADER_IMPLEMENTATION
#include "Headers\tiny_obj_loader.h"
#include "Headers\obj_mesh.h";
#include "Headers\shader.h"
#include "Headers\camera_control.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/norm.hpp>
using namespace glm;

struct Particle {
	glm::vec3 pos, speed;
	int life;
	float size, angle, weight;
	float cameradistance; // *Squared* distance to the camera. if dead : -1.0f

	bool operator<(const Particle& that) const {
		// Sort in reverse order : far particles drawn first.
		return this->cameradistance > that.cameradistance;
	}
};

//-------------------- VARIABLES --------------------\\

GLFWwindow* window;
const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;

const int MaxParticles = 10;
Particle ParticlesContainer[MaxParticles];
int LastUsedParticle = 0;

glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f);	// Initial Camera Position
float horizontalAngle = 3.14f;				// Initial vertical angle : Z axis (3.14)
float verticalAngle = 0.0f;					// Initial vertical angle : none
float initialFoV = 45.0f;					// Initial Field of View
float speed = 3.0f;							// Camera Movement Speed (3 units / second)
float mouseSpeed = 0.005f;					// Camera Rotation Speed

//-------------------- FUNCTIONS --------------------\\

float getDistance(float X_POS_1, float Y_POS_1, float Z_POS_1, float X_POS_2, float Y_POS_2, float Z_POS_2) {
	return sqrt(pow(X_POS_2 - X_POS_1, 2) + pow(Y_POS_2 - Y_POS_1, 2) + pow(Z_POS_2 - Z_POS_1, 2));
};

int FindUnusedParticle() {

	for (int i = LastUsedParticle; i < MaxParticles; i++) {
		if (ParticlesContainer[i].life < 0) {
			LastUsedParticle = i;
			return i;
		}
	}

	for (int i = 0; i < LastUsedParticle; i++) {
		if (ParticlesContainer[i].life < 0) {
			LastUsedParticle = i;
			return i;
		}
	}

	return 0; // All particles are taken, override the first one
}

void SortParticles() {
	std::sort(&ParticlesContainer[0], &ParticlesContainer[MaxParticles]);
}

int main() {

	// Pragma Region: Initialization
	// Description: Initializing window creation using GLFW and GLEW libraries
#pragma region Initialization

	// Initialise GLFW
	if (glfwInit() != GLFW_TRUE) {
		fprintf(stderr, "Failed to initialized! \n");
		return -1;
	}

	glfwWindowHint(GLFW_SAMPLES, 4);
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // To make MacOS happy; should not be needed
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// Open a window and create its OpenGL context
	window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Physics Engine Phase 2 (Ogawa & Urrutia)", NULL, NULL);
	if (window == NULL) {
		fprintf(stderr, "Failed to load window! \n");
		return -1;
	}
	glfwMakeContextCurrent(window);

	// Initialize GLEW
	glewExperimental = true;
	if (glewInit() != GLEW_OK) {
		fprintf(stderr, "Failed to initialize GLEW\n");
		return -1;
	}

#pragma endregion

#pragma region Mesh Loading

	ObjData bullet;
	LoadObjFile(&bullet, "Sphere.obj");
	GLfloat bulletOffsets[] = { 0.0f, 0.0f, 5.0f };
	LoadObjToMemory(&bullet, 0.1f, bulletOffsets);

	ObjData box;
	LoadObjFile(&box, "Box.obj");
	GLfloat boxOffsets[] = { 3.0f, 0.0f, 0.0f };
	LoadObjToMemory(&box, 0.2f, boxOffsets);

#pragma endregion

#pragma region Shader Loading

	GLuint shaderProgram = LoadShaders("Shaders/vertex.shader", "Shaders/fragment.shader");
	glUseProgram(shaderProgram);

	GLuint colorLoc = glGetUniformLocation(shaderProgram, "u_color");
	//glUniform3f(colorLoc, 1.0f, 0.0f, 0.0f);

	// initialize MVP
	GLuint modelTransformLoc = glGetUniformLocation(shaderProgram, "u_model");
	GLuint viewLoc = glGetUniformLocation(shaderProgram, "u_view");
	GLuint projectionLoc = glGetUniformLocation(shaderProgram, "u_projection");

	glm::mat4 trans = glm::mat4(1.0f); // identity
	glUniformMatrix4fv(modelTransformLoc, 1, GL_FALSE, glm::value_ptr(trans));

	// define projection matrix
	glm::mat4 projection = glm::mat4(1.0f);
	//glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

#pragma endregion

	// Ensure we can capture the escape key being pressed below
	glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);

	// Get mouse position
	double xpos, ypos;

	// Set the mouse at the center of the screen
	glfwPollEvents();
	glfwSetCursorPos(window, SCR_WIDTH / 2, SCR_HEIGHT / 2);

	// Setup White Background
	glClearColor(0.9f, 0.9f, 0.9f, 0.0f);

	// Enable depth test
	glEnable(GL_DEPTH_TEST);
	// Accept fragment if it closer to the camera than the former one
	glDepthFunc(GL_LESS);

	GLuint VertexArrayID;
	glGenVertexArrays(1, &VertexArrayID);
	glBindVertexArray(VertexArrayID);

	// glfwGetTime is called only once, the first time this function is called
	static double prevTime = glfwGetTime();

	// Compute time difference between current and last frame
	double currentTime;
	float deltaTime;

	while (glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS && glfwWindowShouldClose(window) == 0) {

#pragma region Viewport

		float ratio;
		int width, height;

		glfwGetFramebufferSize(window, &width, &height);
		ratio = width / (float)height;

		glViewport(0, 0, width, height);

#pragma endregion

#pragma region Camera

		currentTime = glfwGetTime();
		deltaTime = float(currentTime - prevTime);

		glfwGetCursorPos(window, &xpos, &ypos);
		glfwSetCursorPos(window, SCR_WIDTH / 2, SCR_HEIGHT / 2);

		// Compute new orientation
		horizontalAngle += mouseSpeed * float(SCR_WIDTH / 2 - xpos);
		verticalAngle += mouseSpeed * float(SCR_HEIGHT / 2 - ypos);

		// Restriction Camera Angle Vertical
		if (verticalAngle > 0.8f) {
			verticalAngle = 0.8f;
		}

		if (verticalAngle < -0.8f) {
			verticalAngle = -0.8f;
		}

		// Direction : Spherical coordinates to Cartesian coordinates conversion
		glm::vec3 direction(
			cos(verticalAngle) * sin(horizontalAngle),
			sin(verticalAngle),
			cos(verticalAngle) * cos(horizontalAngle)
		);

		// Right vector
		glm::vec3 right = glm::vec3(
			sin(horizontalAngle - 3.14f / 2.0f),
			0,
			cos(horizontalAngle - 3.14f / 2.0f)
		);

		// Up vector
		glm::vec3 up = glm::cross(right, direction);

		// Move forward
		if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
			position += direction * deltaTime * speed;
		}
		// Move backward
		if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
			position -= direction * deltaTime * speed;
		}
		// Strafe right
		if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
			position += right * deltaTime * speed;
		}
		// Strafe left
		if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
			position -= right * deltaTime * speed;
		}

		float FoV = initialFoV;// - 5 * glfwGetMouseWheel(); // Now GLFW 3 requires setting up a callback for this. It's a bit too complicated for this beginner's tutorial, so it's disabled instead.

		// Projection matrix : 45° Field of View, 4:3 ratio, display range : 0.1 unit <-> 100 units
		glm::mat4 projection = glm::perspective(glm::radians(FoV), 4.0f / 3.0f, 0.1f, 100.0f);

		// Camera matrix
		glm::mat4 view = glm::lookAt(
			position,           // Camera is here
			position + direction, // and looks here : at the same position, plus "direction"
			up                  // Head is up (set to 0,-1,0 to look upside-down)
		);

		// For the next frame, the "last time" will be "now"
		prevTime = currentTime;

		printf("%f / %f \n", horizontalAngle, verticalAngle);

		// Set projection matrix in shader
		glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));
		glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

#pragma endregion

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		//toggle to render wit GL_FILL or GL_LINE
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

#pragma region Draw

		//Drawing the Bullet
		glBindVertexArray(bullet.vaoId);

		trans = glm::mat4(1.0f); // identit
		trans = glm::translate(trans, glm::vec3(2.0f, 0.0f, 0.0f));
		trans = glm::scale(trans, glm::vec3(1.0f, 1.0f, 1.0f));

		glActiveTexture(GL_TEXTURE0);
		GLuint bulletTexture = bullet.textures[bullet.materials[0].diffuse_texname];
		glBindTexture(GL_TEXTURE_2D, bulletTexture);

		glUniformMatrix4fv(modelTransformLoc, 1, GL_FALSE, glm::value_ptr(trans));
		glDrawElements(GL_TRIANGLES, bullet.numFaces, GL_UNSIGNED_INT, (void*)0);

		////Drawing the Box
		glBindVertexArray(box.vaoId);

		trans = glm::mat4(1.0f); // identity
		trans = glm::translate(trans, glm::vec3(0.0f, 0.0f, 0.0f));
		trans = glm::scale(trans, glm::vec3(0.5f, 0.5f, 0.5f));

		glActiveTexture(GL_TEXTURE0);
		GLuint boxTexture = box.textures[box.materials[0].diffuse_texname];
		glBindTexture(GL_TEXTURE_2D, boxTexture);

		//Send to shader
		glUniformMatrix4fv(modelTransformLoc, 1, GL_FALSE, glm::value_ptr(trans));
		glDrawElements(GL_TRIANGLES, box.numFaces, GL_UNSIGNED_INT, (void*)0);

#pragma endregion

		glfwSwapBuffers(window);
		//listen for glfw input events
		glfwPollEvents();

	}

	return 0;
}