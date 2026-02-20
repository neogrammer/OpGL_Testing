#pragma once
#include <math.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>


#include <string>

#include <iostream>
#include <learnopengl/shader_s.h>

#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>
#include <learnopengl/camera.h>

class cube_renderer
{
public:
	GLfloat* model;

public:
	glm::vec3 scl{ 1.f };
	glm::vec3 trns{ 0.f,0.f,-1.5f };
	float rotX{ 0.f };
	float rotY{ 0.f };
	float rotZ{ 0.f };
	GLuint texture1;
	GLuint texture2;
	unsigned int VBO, VAO;
	Shader ourShader;


	cube_renderer();
	~cube_renderer();

	void Draw_Cube(glm::mat4& projection, glm::mat4& view, Camera& camera, int SCR_WIDTH, int SCR_HEIGHT);

};
