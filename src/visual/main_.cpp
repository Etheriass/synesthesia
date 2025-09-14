#include <thread>
#include <random>
#include <iostream>

#include "draw.h"
#include "shared_state.h"


void flux_input_thread(SharedState *shared)
{
    std::mt19937 rng((unsigned)std::hash<std::thread::id>{}(std::this_thread::get_id()));
    std::uniform_real_distribution<float> ux_dist(0.1f, 0.9f);
    std::uniform_real_distribution<float> uy_dist(0.1f, 0.9f);
    std::uniform_real_distribution<float> radius_dist(0.02f, 0.1f);
    std::uniform_real_distribution<float> falloff_dist(0.8f, 2.0f);
    std::uniform_real_distribution<float> intensity_dist(0.5f, 1.5f);

    while (shared->running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        if (!shared->running) break;
        float ux = ux_dist(rng);
        float uy = uy_dist(rng);
        float r = radius_dist(rng);
        float fo = falloff_dist(rng);
        float it = intensity_dist(rng);
        std::cout << "Flux: adding circle at " << ux << "," << uy << " r=" << r << " fo=" << fo << " it=" << it << "\n";
        add_circle_shared(shared, ux, uy, r, fo, it);
    }
}

int main()
{
    SharedState shared;

    // 2 threads: one for drawing, one for flux input
    std::thread draw_thread(drawing_thread, &shared);
    std::thread flux_thread(flux_input_thread, &shared);
    draw_thread.join();
    // drawing thread signals running=false when it exits; make sure flux thread can finish
    if (flux_thread.joinable())
        flux_thread.join();

    return 0;
}
