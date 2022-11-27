/*
	HOMEWORK: PHYSICS ENGINE PHASE 2
	MEMBERS: Junma Ogawa
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

GLFWwindow* window;

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
	window = glfwCreateWindow(1200, 800, "Physics Engine Phase 2 (Ogawa & Urrutia)", NULL, NULL);
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
	GLfloat bulletOffsets[] = { 0.0f, 0.0f, 0.0f };
	LoadObjToMemory(&bullet, 1.0f, bulletOffsets);

#pragma endregion

#pragma region Shader Loading

	GLuint shaderProgram = LoadShaders("Shaders/vertex.shader", "Shaders/fragment.shader");
	glUseProgram(shaderProgram);

	GLuint colorLoc = glGetUniformLocation(shaderProgram, "u_color");
	glUniform3f(colorLoc, 1.0f, 0.0f, 0.0f);

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

	// Set the mouse at the center of the screen
	glfwPollEvents();
	glfwSetCursorPos(window, 1024 / 2, 768 / 2);

	// Setup White Background
	glClearColor(1.0f, 1.0f, 1.0f, 0.0f);

	// Enable depth test
	glEnable(GL_DEPTH_TEST);
	// Accept fragment if it closer to the camera than the former one
	glDepthFunc(GL_LESS);

	GLuint VertexArrayID;
	glGenVertexArrays(1, &VertexArrayID);
	glBindVertexArray(VertexArrayID);

	while (!glfwWindowShouldClose(window)) {

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		//toggle to render wit GL_FILL or GL_LINE
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

#pragma region Draw

		//Drawing the Bullet
		glBindVertexArray(bullet.vaoId);

		//Adjust Bullet Position, Size, & Color
		trans = glm::mat4(1.0f); // identity
		trans = glm::translate(trans, glm::vec3(0.0f, 0.0f, 0.0f));
		trans = glm::scale(trans, glm::vec3(1.0f, 1.0f, 1.0f));
		glUniform3f(colorLoc, 0.0f, 0.0f, 0.0f);
		
		//Send to shader
		glUniformMatrix4fv(modelTransformLoc, 1, GL_FALSE, glm::value_ptr(trans));

		glDrawElements(GL_TRIANGLES, bullet.numFaces, GL_UNSIGNED_INT,(void*)0);

#pragma endregion

		glfwSwapBuffers(window);
		//listen for glfw input events
		glfwPollEvents();

	}

	return 0;
}