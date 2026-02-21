#pragma once


#include <glad/glad.h>
#include <GLFW/glfw3.h>

class App
{
	GLFWwindow* window;
	
	static void OnResize(int x, int y);
	static void OnMouse(double x, double y);
	static void OnScroll(double x, double y);

public:
	void InitWindow(GLFWwindow* window_);
	bool Run();
};