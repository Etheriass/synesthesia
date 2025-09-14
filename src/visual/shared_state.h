#ifndef SHARED_STATE_H
#define SHARED_STATE_H

#include <mutex>
#include <atomic>
#include <GLFW/glfw3.h>

#include "circle.h"

struct SharedState {
    std::vector<Circle> circles;
    std::mutex mtx;
    std::atomic<bool> running{true};
};

// Thread-safe helper to add a circle to shared state
static inline void add_circle_shared(SharedState *shared, float ux, float uy, float radius = 0.05f, float falloff = 1.4f, float intensity = 1.0f)
{
    if (!shared)
        return;
    Circle c;
    c.x = ux;
    c.y = uy;
    // don't call glfwGetTime() from the flux thread; set t0=0 and let the drawing
    // thread initialize it to the correct time when it copies the list.
    c.t0 = 0.0;
    c.radius = radius;
    c.falloff = falloff;
    c.intensity = intensity;

    std::lock_guard<std::mutex> lk(shared->mtx);
    if (shared->circles.size() >= (size_t)kMaxCircles)
        shared->circles.erase(shared->circles.begin());
    shared->circles.push_back(c);
}

// Backwards-compatible: add circle using the GLFW window's user pointer (if it points to SharedState)
inline void add_circle(GLFWwindow *win, float ux, float uy, float radius = 0.05f, float falloff = 1.4f, float intensity = 1.0f)
{
    if (!win)
        return;
    auto sp = static_cast<SharedState *>(glfwGetWindowUserPointer(win));
    if (!sp)
        return;
    add_circle_shared(sp, ux, uy, radius, falloff, intensity);
}

#endif
