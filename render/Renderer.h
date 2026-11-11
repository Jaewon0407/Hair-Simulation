#pragma once

#include <cstddef>

struct GLFWwindow;

namespace render {

struct Color {
    float r, g, b;
};

// Minimal OpenGL 4.1 viewer for hair particles and strands.
//
// The renderer does not know about any simulation types. Position data is
// described by a pointer + count + stride, so it works with any layout as long
// as each position is 3 contiguous floats (x, y, z):
//
//   tightly packed floats / Vec3 array:  draw*(ptr, count, 3 * sizeof(float))
//   position member inside a struct:     draw*(&particles[0].position,
//                                              particles.size(), sizeof(Particle))
//
// Controls: left-drag = orbit, scroll = zoom, Esc = close.
class Renderer {
public:
    Renderer(int width, int height);
    ~Renderer();

    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    bool isOpen() const;

    // Call once per frame, in this order: beginFrame -> draw* -> endFrame.
    void beginFrame();
    void endFrame();

    void drawPoints(const void* firstPosition, std::size_t count, std::size_t strideBytes,
                    Color color = {0.95f, 0.75f, 0.35f}, float sizePixels = 6.0f);

    // Connects consecutive positions with lines (one call per strand).
    void drawLineStrip(const void* firstPosition, std::size_t count, std::size_t strideBytes,
                       Color color = {0.6f, 0.45f, 0.25f});

    // Reference grid on the y = 0 plane.
    void drawGrid(float halfSize = 1.0f, int divisions = 20,
                  Color color = {0.25f, 0.25f, 0.28f});

    // Camera orbits around `target` at `distance`.
    void setCameraTarget(float x, float y, float z);
    void setCameraDistance(float distance);

    bool isKeyDown(int glfwKey) const;
    double time() const;  // seconds since start
    GLFWwindow* window() const { return window_; }

private:
    void draw(unsigned mode, const void* data, std::size_t count, std::size_t strideBytes,
              Color color, float pointSize, bool roundPoints);

    static void cursorPosCallback(GLFWwindow* w, double x, double y);
    static void scrollCallback(GLFWwindow* w, double dx, double dy);
    static void keyCallback(GLFWwindow* w, int key, int scancode, int action, int mods);

    GLFWwindow* window_ = nullptr;
    unsigned program_ = 0;
    unsigned vao_ = 0;
    unsigned vbo_ = 0;
    int uViewProj_ = -1;
    int uColor_ = -1;
    int uPointSize_ = -1;
    int uRoundPoints_ = -1;

    float viewProj_[16] = {};
    float pixelScale_ = 1.0f;  // framebuffer px per window px (2 on Retina)

    float target_[3] = {0.0f, 0.0f, 0.0f};
    float distance_ = 2.0f;
    float yaw_ = 0.6f;     // radians
    float pitch_ = 0.35f;  // radians
    double lastX_ = 0.0;
    double lastY_ = 0.0;
    bool haveLastCursor_ = false;
};

}  // namespace render
