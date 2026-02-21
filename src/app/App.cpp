#include "App.h"

void App::OnResize(int x, int y)
{
}

void App::OnMouse(double x, double y)
{
}

void App::OnScroll(double x, double y)
{
}

void App::InitWindow(GLFWwindow* window_)
{
	glfwSetWindowUserPointer(window_, this);

	glfwSetFramebufferSizeCallback(window_, [](GLFWwindow* w, int a, int b) {
		static_cast<App*>(glfwGetWindowUserPointer(w))->OnResize(a, b);
		});
	glfwSetCursorPosCallback(window_, [](GLFWwindow* w, double x, double y) {
		static_cast<App*>(glfwGetWindowUserPointer(w))->OnMouse(x, y);
		});
	glfwSetScrollCallback(window_, [](GLFWwindow* w, double x, double y) {
		static_cast<App*>(glfwGetWindowUserPointer(w))->OnScroll(x, y);
		});
	
	window = window_;
}

bool App::Run()
{
	return true;
}
