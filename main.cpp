#include <iostream>
#include "Renderer.h"

int main() {

    int width = 1280;
    int height = 720;
    render::Renderer renderer(width, height);

    renderer.setCameraTarget(0.0f, 0.5f, 0.0f);   // where the camera looks
    renderer.setCameraDistance(2.0f);

    while (renderer.isOpen()) 
    {
        renderer.beginFrame();
        renderer.drawGrid();
        renderer.drawLineStrip(positions.data(), positions.size() / 3, 3 * sizeof(float));
        renderer.drawPoints(positions.data(), positions.size() / 3, 3 * sizeof(float));
        renderer.endFrame();
    }

    return 0;
}