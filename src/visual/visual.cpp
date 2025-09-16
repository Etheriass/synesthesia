#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <filesystem>

#include "visual.h"
#include "init.h"
#include "loader.h"
#include "circle.h"
#include "../shared_state.h"

void visual_thread(SharedState *shared)
{
    GLFWwindow *win = init_window();

    GLuint prog = LoadShaderProgram(
        "src/shaders/circle.vert",
        "src/shaders/circle.frag");

    GLuint vao = 0;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    // Uniform locations
    GLint locCount = glGetUniformLocation(prog, "uCount");
    GLint locPos = glGetUniformLocation(prog, "uPos");
    GLint locAge = glGetUniformLocation(prog, "uAge");
    GLint locLife = glGetUniformLocation(prog, "uLife");
    GLint locRadius = glGetUniformLocation(prog, "uRadius");
    GLint locDotSpacing = glGetUniformLocation(prog, "uDotSpacing");
    GLint locDotRadius = glGetUniformLocation(prog, "uDotRadius");
    // per-circle params
    GLint locRadiusArr = glGetUniformLocation(prog, "uRadiusArr");
    GLint locFalloffArr = glGetUniformLocation(prog, "uFalloffArr");
    GLint locIntensityArr = glGetUniformLocation(prog, "uIntensityArr");

    // attach shared state to the window so legacy add_circle(win,...) works
    glfwSetWindowUserPointer(win, shared);

    double t0 = glfwGetTime();

    // Render loop
    while (!glfwWindowShouldClose(win))
    {
        glfwPollEvents();

        int w, h;
        glfwGetFramebufferSize(win, &w, &h);
        glViewport(0, 0, w, h);

        // Black background
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        double now = glfwGetTime();

        // Remove expired circles under lock and copy current list to a local vector
        std::vector<Circle> localCircles;
        {
            std::lock_guard<std::mutex> lk(shared->mtx);
            // initialize t0 for any circles added by flux (t0==0)
            for (Circle &c : shared->circles) {
                if (c.t0 <= 0.0)
                    c.t0 = now;
            }
            for (size_t i = 0; i < shared->circles.size();) {
                if (now - shared->circles[i].t0 >= kCircleLife)
                    shared->circles.erase(shared->circles.begin() + i);
                else
                    ++i;
            }
            localCircles = shared->circles; // copy for rendering work without holding lock
        }

        // Prepare arrays for uniforms from local copy
        int count = (int)localCircles.size();
        std::vector<float> posData;
        posData.reserve(count * 2);
        std::vector<float> ageData;
        ageData.reserve(count);
        std::vector<float> radiusData;
        radiusData.reserve(count);
        std::vector<float> falloffData;
        falloffData.reserve(count);
        std::vector<float> intensityData;
        intensityData.reserve(count);
        for (const Circle &c : localCircles)
        {
            posData.push_back(c.x);
            posData.push_back(c.y);
            ageData.push_back(float(now - c.t0));
            radiusData.push_back(c.radius);
            falloffData.push_back(c.falloff);
            intensityData.push_back(c.intensity);
        }

        // Use program and upload uniforms
        glUseProgram(prog);
        glUniform1i(locCount, count);
        glUniform1f(locLife, float(kCircleLife));
        // set dotted-circle params (tweak these to taste)
        float radius = 0.05f;      // main circle radius in UV
        float dotSpacing = 0.001f; // spacing between dots in UV
        float dotRadius = 0.0005f; // small dot radius in UV
        if (locRadius >= 0)
            glUniform1f(locRadius, radius);
        if (locDotSpacing >= 0)
            glUniform1f(locDotSpacing, dotSpacing);
        if (locDotRadius >= 0)
            glUniform1f(locDotRadius, dotRadius);
        if (count > 0)
        {
            // upload vec2 array (count elements)
            glUniform2fv(locPos, count, posData.data());
            glUniform1fv(locAge, count, ageData.data());
            if (locRadiusArr >= 0)
                glUniform1fv(locRadiusArr, count, radiusData.data());
            if (locFalloffArr >= 0)
                glUniform1fv(locFalloffArr, count, falloffData.data());
            if (locIntensityArr >= 0)
                glUniform1fv(locIntensityArr, count, intensityData.data());
        }

        // Draw full-screen triangle (3 verts)
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        glfwSwapBuffers(win);
    }

    // Cleanup
    glDeleteVertexArrays(1, &vao);
    glDeleteProgram(prog);

    glfwDestroyWindow(win);
    glfwTerminate();
    // signal flux thread to stop
    shared->running = false;
}

