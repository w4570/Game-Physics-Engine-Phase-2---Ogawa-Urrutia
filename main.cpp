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
#include "Headers\control.h"
#include "Headers\shader.h"
#include "Headers\texture.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/norm.hpp>
using namespace glm;

//-------------------- STRUCTURE --------------------\\

// CPU representation of a particle
struct Particle {
	glm::vec3 pos, speed;
	unsigned char r, g, b, a; // Color
	float size, angle, drag;
	float life; // Remaining life of the particle. if <0 : dead and unused.
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
float particleSize = 0.1f;
glm::vec3 bulletOrigin = glm::vec3(-8.0f, 0.0f, 0.0f);
float xForce = 0.0f, yForce = 0.0f, gravity = 0.0f;
bool renderParticle = false;

//-------------------- FUNCTIONS --------------------\\

float getDistance(float X_POS_1, float Y_POS_1, float Z_POS_1, float X_POS_2, float Y_POS_2, float Z_POS_2) {
	return sqrt(pow(X_POS_2 - X_POS_1, 2) + pow(Y_POS_2 - Y_POS_1, 2) + pow(Z_POS_2 - Z_POS_1, 2));
};

// Finds a Particle in ParticlesContainer which isn't used yet.
// (i.e. life < 0);
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

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	// switch case for changing projectile type
	switch (key) {
		//normal heavy projectile
	case '1':    // Weak Bullet, Gravity Disabled
		xForce = 5.0f;
		yForce = 0.0f;
		gravity = 0.0f;
		break;
	case '2': // Strong Bullet, Gravity Disabled
		xForce = 12.0f;
		yForce = 0.0f;
		gravity = 0.0f;
		break;
	case '3': // Cannon Bullet, Gravity Enabled
		xForce = 8.0f;
		yForce = 4.0f;
		gravity = -9.8f;
		break;
	default:
		printf("Invalid Input");
		break;
	}
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

	float boxSize = 0.3f, boxMass = 10.0f;
	glm::vec3 boxPosition = glm::vec3(0.0f, 0.0f, 0.0f);

	ObjData bullet;
	LoadObjFile(&bullet, "Sphere.obj");
	GLfloat bulletOffsets[] = { 0.0f, 0.0f, 0.0f };
	LoadObjToMemory(&bullet, particleSize, bulletOffsets);

	ObjData box;
	LoadObjFile(&box, "Box.obj");
	GLfloat boxOffsets[] = { 0.0f, 0.0f, 0.0f };
	LoadObjToMemory(&box, boxSize, boxOffsets);

#pragma endregion

#pragma region Shader Loading

	GLuint shaderProgram = LoadShaders("Shaders/vertex.shader", "Shaders/fragment.shader");
	glUseProgram(shaderProgram);

	GLuint objectShaderProgram = LoadShaders("Shaders/objectVertex.shader", "Shaders/objectFragment.shader");
	glUseProgram(objectShaderProgram);

	GLuint colorLoc = glGetUniformLocation(objectShaderProgram, "u_color");
	//glUniform3f(colorLoc, 1.0f, 0.0f, 0.0f);

	// initialize MVP
	GLuint modelTransformLoc = glGetUniformLocation(objectShaderProgram, "u_model");
	GLuint viewLoc = glGetUniformLocation(objectShaderProgram, "u_view");
	GLuint projectionLoc = glGetUniformLocation(objectShaderProgram, "u_projection");

	glm::mat4 trans = glm::mat4(1.0f); // identity
	glUniformMatrix4fv(modelTransformLoc, 1, GL_FALSE, glm::value_ptr(trans));

	// define projection matrix
	glm::mat4 projection = glm::mat4(1.0f);

	// Vertex shader
	GLuint CameraRight_worldspace_ID = glGetUniformLocation(shaderProgram, "CameraRight_worldspace");
	GLuint CameraUp_worldspace_ID = glGetUniformLocation(shaderProgram, "CameraUp_worldspace");
	GLuint ViewProjMatrixID = glGetUniformLocation(shaderProgram, "VP");

	// fragment shader
	GLuint TextureID = glGetUniformLocation(shaderProgram, "myTextureSampler");

#pragma endregion

	for (int i = 0; i < MaxParticles; i++) {
		ParticlesContainer[i].life = -1.0f;
		ParticlesContainer[i].cameradistance = -1.0f;
	}

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

	static GLfloat* g_particule_position_size_data = new GLfloat[MaxParticles * 4];
	static GLubyte* g_particule_color_data = new GLubyte[MaxParticles * 4];

	for (int i = 0; i < MaxParticles; i++) {
		ParticlesContainer[i].life = -1.0f;
		ParticlesContainer[i].cameradistance = -1.0f;
		ParticlesContainer[i].pos = bulletOrigin;
	}

	GLuint Texture = loadDDS("particle.DDS");

	// The VBO containing the 4 vertices of the particles.
	// Thanks to instancing, they will be shared by all particles.
	static const GLfloat g_vertex_buffer_data[] = {
		 -0.5f, -0.5f, 0.0f,
		  0.5f, -0.5f, 0.0f,
		 -0.5f,  0.5f, 0.0f,
		  0.5f,  0.5f, 0.0f,
	};
	GLuint billboard_vertex_buffer;
	glGenBuffers(1, &billboard_vertex_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, billboard_vertex_buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(g_vertex_buffer_data), g_vertex_buffer_data, GL_STATIC_DRAW);

	// The VBO containing the positions and sizes of the particles
	GLuint particles_position_buffer;
	glGenBuffers(1, &particles_position_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, particles_position_buffer);
	// Initialize with empty (NULL) buffer : it will be updated later, each frame.
	glBufferData(GL_ARRAY_BUFFER, MaxParticles * 4 * sizeof(GLfloat), NULL, GL_STREAM_DRAW);

	// The VBO containing the colors of the particles
	GLuint particles_color_buffer;
	glGenBuffers(1, &particles_color_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, particles_color_buffer);
	// Initialize with empty (NULL) buffer : it will be updated later, each frame.
	glBufferData(GL_ARRAY_BUFFER, MaxParticles * 4 * sizeof(GLubyte), NULL, GL_STREAM_DRAW);

	float xDisplacement = 0.0f;

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
		prevTime = currentTime;

		computeMatricesFromInputs();
		glm::mat4 ProjectionMatrix = getProjectionMatrix();
		glm::mat4 ViewMatrix = getViewMatrix();

		glm::vec3 CameraPosition(glm::inverse(ViewMatrix)[3]);
		glm::mat4 ViewProjectionMatrix = ProjectionMatrix * ViewMatrix;

#pragma endregion

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		//toggle to render wit GL_FILL or GL_LINE
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		glfwSetKeyCallback(window, key_callback);

#pragma region Draw

		//Drawing the Bullet
		glBindVertexArray(bullet.vaoId);

		trans = glm::mat4(1.0f); // identity
		trans = glm::translate(trans, bulletOrigin);
		trans = glm::scale(trans, glm::vec3(1.0f, 1.0f, 1.0f));

		glActiveTexture(GL_TEXTURE0);
		GLuint bulletTexture = bullet.textures[bullet.materials[0].diffuse_texname];
		glBindTexture(GL_TEXTURE_2D, bulletTexture);

		glUniformMatrix4fv(modelTransformLoc, 1, GL_FALSE, glm::value_ptr(trans));
		glDrawElements(GL_TRIANGLES, bullet.numFaces, GL_UNSIGNED_INT, (void*)0);

		//Drawing the Box
		glBindVertexArray(box.vaoId);

		trans = glm::mat4(1.0f); // identity
		trans = glm::rotate(trans, glm::radians(270.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		trans = glm::scale(trans, glm::vec3(1.0f, 1.0f, 1.0f));
		trans = glm::translate(trans, boxPosition);

		glActiveTexture(GL_TEXTURE0);
		GLuint boxTextureA = box.textures[box.materials[0].diffuse_texname];
		glBindTexture(GL_TEXTURE_2D, boxTextureA);

		//Send to shader
		glUniformMatrix4fv(modelTransformLoc, 1, GL_FALSE, glm::value_ptr(trans));
		glDrawElements(GL_TRIANGLES, box.numFaces, GL_UNSIGNED_INT, (void*)0);

#pragma endregion

#pragma region Particle

		int state = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
		if (state == GLFW_PRESS && renderParticle == false)
		{

			renderParticle = true;

			int particleIndex = FindUnusedParticle();

			float spread = 1.0f;

			glm::vec3 maindir = glm::vec3(xForce, yForce, 0.0f);
			ParticlesContainer[particleIndex].speed = maindir * spread;

			ParticlesContainer[particleIndex].r = 255;
			ParticlesContainer[particleIndex].g = 0;
			ParticlesContainer[particleIndex].b = 0;
			ParticlesContainer[particleIndex].a = 255;

			ParticlesContainer[particleIndex].size = particleSize;
			ParticlesContainer[particleIndex].drag = gravity;
			ParticlesContainer[particleIndex].life = 10.0f;
			//ParticlesContainer[particleIndex].pos = bulletOrigin;

		}
		else if (state == GLFW_RELEASE)
		{
			renderParticle = false;
		}

		int ParticlesCount = 0;
		Particle* newParticle = NULL;

		glfwSetKeyCallback(window, key_callback);

		for (int i = 0; i < MaxParticles; i++) {

			Particle& p = ParticlesContainer[i]; // shortcut

			if (p.life > 0.0f) 
			{
				// Decrease life
				p.life -= deltaTime;

				if (getDistance(p.pos.x, boxPosition.x, p.pos.y, boxPosition.y, p.pos.z, boxPosition.z) <= (p.size / 2) + (boxSize / 2)) {
					printf("Collision\n");
					p.life = -1.0f;
					p.pos = bulletOrigin;
				}

				if (p.life > 0.0f) {

					p.speed += glm::vec3(0.0f, p.drag, 0.0f) * (float)deltaTime * 0.5f;

					// Simulate simple physics : gravity only, no collisions
					p.pos += p.speed * (float)deltaTime;
					p.cameradistance = glm::length2(p.pos - CameraPosition);
					//ParticlesContainer[i].pos += glm::vec3(0.0f,10.0f, 0.0f) * (float)delta;

					// Fill the GPU buffer
					g_particule_position_size_data[4 * ParticlesCount + 0] = p.pos.x;
					g_particule_position_size_data[4 * ParticlesCount + 1] = p.pos.y;
					g_particule_position_size_data[4 * ParticlesCount + 2] = p.pos.z;

					g_particule_position_size_data[4 * ParticlesCount + 3] = p.size;

					g_particule_color_data[4 * ParticlesCount + 0] = p.r;
					g_particule_color_data[4 * ParticlesCount + 1] = p.g;
					g_particule_color_data[4 * ParticlesCount + 2] = p.b;
					g_particule_color_data[4 * ParticlesCount + 3] = p.a;

				}
				else {
					// Particles that just died will be put at the end of the buffer in SortParticles();
					p.cameradistance = -1.0f;
				}

				ParticlesCount++;
			}	
		}

#pragma endregion

		// Update the buffers that OpenGL uses for rendering.
		// There are much more sophisticated means to stream data from the CPU to the GPU, 
		// but this is outside the scope of this tutorial.
		// http://www.opengl.org/wiki/Buffer_Object_Streaming


		glBindBuffer(GL_ARRAY_BUFFER, particles_position_buffer);
		glBufferData(GL_ARRAY_BUFFER, MaxParticles * 4 * sizeof(GLfloat), NULL, GL_STREAM_DRAW); // Buffer orphaning, a common way to improve streaming perf. See above link for details.
		glBufferSubData(GL_ARRAY_BUFFER, 0, ParticlesCount * sizeof(GLfloat) * 4, g_particule_position_size_data);

		glBindBuffer(GL_ARRAY_BUFFER, particles_color_buffer);
		glBufferData(GL_ARRAY_BUFFER, MaxParticles * 4 * sizeof(GLubyte), NULL, GL_STREAM_DRAW); // Buffer orphaning, a common way to improve streaming perf. See above link for details.
		glBufferSubData(GL_ARRAY_BUFFER, 0, ParticlesCount * sizeof(GLubyte) * 4, g_particule_color_data);

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		// Use our shader
		glUseProgram(shaderProgram);

		// Bind our texture in Texture Unit 0
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, Texture);
		// Set our "myTextureSampler" sampler to use Texture Unit 0
		glUniform1i(TextureID, 0);

		// Same as the billboards tutorial
		glUniform3f(CameraRight_worldspace_ID, ViewMatrix[0][0], ViewMatrix[1][0], ViewMatrix[2][0]);
		glUniform3f(CameraUp_worldspace_ID, ViewMatrix[0][1], ViewMatrix[1][1], ViewMatrix[2][1]);

		glUniformMatrix4fv(ViewProjMatrixID, 1, GL_FALSE, &ViewProjectionMatrix[0][0]);

		// 1rst attribute buffer : vertices
		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, billboard_vertex_buffer);
		glVertexAttribPointer(
			0,                  // attribute. No particular reason for 0, but must match the layout in the shader.
			3,                  // size
			GL_FLOAT,           // type
			GL_FALSE,           // normalized?
			0,                  // stride
			(void*)0            // array buffer offset
		);

		// 2nd attribute buffer : positions of particles' centers
		glEnableVertexAttribArray(1);
		glBindBuffer(GL_ARRAY_BUFFER, particles_position_buffer);
		glVertexAttribPointer(
			1,                                // attribute. No particular reason for 1, but must match the layout in the shader.
			4,                                // size : x + y + z + size => 4
			GL_FLOAT,                         // type
			GL_FALSE,                         // normalized?
			0,                                // stride
			(void*)0                          // array buffer offset
		);

		// 3rd attribute buffer : particles' colors
		glEnableVertexAttribArray(2);
		glBindBuffer(GL_ARRAY_BUFFER, particles_color_buffer);
		glVertexAttribPointer(
			2,                                // attribute. No particular reason for 1, but must match the layout in the shader.
			4,                                // size : r + g + b + a => 4
			GL_UNSIGNED_BYTE,                 // type
			GL_TRUE,                          // normalized?    *** YES, this means that the unsigned char[4] will be accessible with a vec4 (floats) in the shader ***
			0,                                // stride
			(void*)0                          // array buffer offset
		);

		// These functions are specific to glDrawArrays*Instanced*.
		// The first parameter is the attribute buffer we're talking about.
		// The second parameter is the "rate at which generic vertex attributes advance when rendering multiple instances"
		// http://www.opengl.org/sdk/docs/man/xhtml/glVertexAttribDivisor.xml
		glVertexAttribDivisor(0, 0); // particles vertices : always reuse the same 4 vertices -> 0
		glVertexAttribDivisor(1, 1); // positions : one per quad (its center)                 -> 1
		glVertexAttribDivisor(2, 1); // color : one per quad                                  -> 1

		// Draw the particules !
		// This draws many times a small triangle_strip (which looks like a quad).
		// This is equivalent to :
		// for(i in ParticlesCount) : glDrawArrays(GL_TRIANGLE_STRIP, 0, 4), 
		// but faster.
		glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, ParticlesCount);

		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);
		glDisableVertexAttribArray(2);

		glfwSwapBuffers(window);
		//listen for glfw input events
		glfwPollEvents();

	}

	delete[] g_particule_position_size_data;

	// Cleanup VBO and shader
	glDeleteBuffers(1, &particles_color_buffer);
	glDeleteBuffers(1, &particles_position_buffer);
	glDeleteBuffers(1, &billboard_vertex_buffer);
	glDeleteProgram(shaderProgram);
	glDeleteTextures(1, &Texture);
	glDeleteVertexArrays(1, &VertexArrayID);

	// Close OpenGL window and terminate GLFW
	glfwTerminate();

	return 0;
}