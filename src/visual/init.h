#ifndef INIT_H
#define INIT_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stdexcept>

inline GLFWwindow *init_window()
{
    if (!glfwInit())
    {
        std::fprintf(stderr, "glfwInit failed\n");
        std::exit(1);
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#if __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    glfwWindowHint(GLFW_DECORATED, GLFW_FALSE); // no window border

    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);

    // GLFWwindow *win = glfwCreateWindow(mode->width, mode->height, "Visualizer", monitor, nullptr);
    GLFWwindow *win = glfwCreateWindow(1000, 1000, "Visualizer", nullptr, nullptr);
    if (!win)
    {
        std::fprintf(stderr, "glfwCreateWindow failed\n");
        glfwTerminate();
        std::exit(1);
    }

    glfwMakeContextCurrent(win);
    glfwSwapInterval(1);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::fprintf(stderr, "Failed to initialize GLAD\n");
        std::exit(1);
    }

    std::fprintf(stderr, "GL_VERSION  : %s\n", glGetString(GL_VERSION));
    std::fprintf(stderr, "GL_RENDERER : %s\n", glGetString(GL_RENDERER));
    return win;
}

#endif