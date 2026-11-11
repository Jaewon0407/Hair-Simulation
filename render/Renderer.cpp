#include "Renderer.h"

#include <glad/gl.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <stdexcept>
#include <string>
#include <vector>

namespace render {
namespace {

// ---- Tiny internal math (column-major, OpenGL convention) -------------------
struct V3 {
    float x, y, z;
};

V3 sub(V3 a, V3 b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
float dot(V3 a, V3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
V3 cross(V3 a, V3 b) { return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x}; }
V3 normalize(V3 v) {
    const float len = std::sqrt(dot(v, v));
    return len > 0.0f ? V3{v.x / len, v.y / len, v.z / len} : v;
}

void multiply(const float* a, const float* b, float* out) {
    for (int col = 0; col < 4; ++col)
        for (int row = 0; row < 4; ++row) {
            float s = 0.0f;
            for (int k = 0; k < 4; ++k) s += a[k * 4 + row] * b[col * 4 + k];
            out[col * 4 + row] = s;
        }
}

void perspective(float fovyRad, float aspect, float zNear, float zFar, float* m) {
    const float f = 1.0f / std::tan(fovyRad * 0.5f);
    std::fill(m, m + 16, 0.0f);
    m[0] = f / aspect;
    m[5] = f;
    m[10] = (zFar + zNear) / (zNear - zFar);
    m[11] = -1.0f;
    m[14] = (2.0f * zFar * zNear) / (zNear - zFar);
}

void lookAt(V3 eye, V3 target, V3 up, float* m) {
    const V3 f = normalize(sub(target, eye));
    const V3 s = normalize(cross(f, up));
    const V3 u = cross(s, f);
    m[0] = s.x;  m[4] = s.y;  m[8] = s.z;   m[12] = -dot(s, eye);
    m[1] = u.x;  m[5] = u.y;  m[9] = u.z;   m[13] = -dot(u, eye);
    m[2] = -f.x; m[6] = -f.y; m[10] = -f.z; m[14] = dot(f, eye);
    m[3] = 0.0f; m[7] = 0.0f; m[11] = 0.0f; m[15] = 1.0f;
}

// ---- Shaders ----------------------------------------------------------------
const char* kVertexSrc = R"(#version 410 core
layout(location = 0) in vec3 aPos;
uniform mat4 uViewProj;
uniform float uPointSize;
void main() {
    gl_Position = uViewProj * vec4(aPos, 1.0);
    gl_PointSize = uPointSize;
}
)";

const char* kFragmentSrc = R"(#version 410 core
uniform vec3 uColor;
uniform int uRoundPoints;
out vec4 fragColor;
void main() {
    if (uRoundPoints == 1) {
        vec2 d = gl_PointCoord * 2.0 - 1.0;
        float r2 = dot(d, d);
        if (r2 > 1.0) discard;
        float shade = 0.55 + 0.45 * sqrt(1.0 - r2);  // fake sphere shading
        fragColor = vec4(uColor * shade, 1.0);
    } else {
        fragColor = vec4(uColor, 1.0);
    }
}
)";

GLuint compileShader(GLenum type, const char* src) {
    const GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);
    GLint ok = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
        glDeleteShader(shader);
        throw std::runtime_error(std::string("Shader compile failed: ") + log);
    }
    return shader;
}

GLuint linkProgram(const char* vsSrc, const char* fsSrc) {
    const GLuint vs = compileShader(GL_VERTEX_SHADER, vsSrc);
    const GLuint fs = compileShader(GL_FRAGMENT_SHADER, fsSrc);
    const GLuint program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);
    glDeleteShader(vs);
    glDeleteShader(fs);
    GLint ok = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetProgramInfoLog(program, sizeof(log), nullptr, log);
        glDeleteProgram(program);
        throw std::runtime_error(std::string("Program link failed: ") + log);
    }
    return program;
}

void glfwErrorCallback(int code, const char* description) {
    std::fprintf(stderr, "GLFW error %d: %s\n", code, description);
}

constexpr float kPi = 3.14159265358979f;

}  // namespace

// ---- Renderer ---------------------------------------------------------------
Renderer::Renderer(int width, int height, const char* title) {
    glfwSetErrorCallback(glfwErrorCallback);
    if (!glfwInit()) throw std::runtime_error("glfwInit failed");

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);  // required on macOS
    glfwWindowHint(GLFW_SAMPLES, 4);

    window_ = glfwCreateWindow(width, height, title, nullptr, nullptr);
    if (!window_) {
        glfwTerminate();
        throw std::runtime_error("glfwCreateWindow failed");
    }
    glfwMakeContextCurrent(window_);
    glfwSwapInterval(1);  // vsync

    if (!gladLoadGL(glfwGetProcAddress)) {
        glfwDestroyWindow(window_);
        glfwTerminate();
        throw std::runtime_error("gladLoadGL failed");
    }

    glfwSetWindowUserPointer(window_, this);
    glfwSetCursorPosCallback(window_, cursorPosCallback);
    glfwSetScrollCallback(window_, scrollCallback);
    glfwSetKeyCallback(window_, keyCallback);

    program_ = linkProgram(kVertexSrc, kFragmentSrc);
    uViewProj_ = glGetUniformLocation(program_, "uViewProj");
    uColor_ = glGetUniformLocation(program_, "uColor");
    uPointSize_ = glGetUniformLocation(program_, "uPointSize");
    uRoundPoints_ = glGetUniformLocation(program_, "uRoundPoints");

    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);
    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glEnableVertexAttribArray(0);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_PROGRAM_POINT_SIZE);
    glEnable(GL_MULTISAMPLE);
}

Renderer::~Renderer() {
    if (window_) {
        glfwMakeContextCurrent(window_);
        glDeleteBuffers(1, &vbo_);
        glDeleteVertexArrays(1, &vao_);
        glDeleteProgram(program_);
        glfwDestroyWindow(window_);
    }
    glfwTerminate();
}

bool Renderer::isOpen() const { return !glfwWindowShouldClose(window_); }

void Renderer::beginFrame() {
    glfwPollEvents();

    int fbW = 0, fbH = 0, winW = 0, winH = 0;
    glfwGetFramebufferSize(window_, &fbW, &fbH);
    glfwGetWindowSize(window_, &winW, &winH);
    pixelScale_ = winW > 0 ? static_cast<float>(fbW) / static_cast<float>(winW) : 1.0f;

    glViewport(0, 0, fbW, fbH);
    glClearColor(0.08f, 0.08f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    const V3 target{target_[0], target_[1], target_[2]};
    const V3 eye{target.x + distance_ * std::cos(pitch_) * std::sin(yaw_),
                 target.y + distance_ * std::sin(pitch_),
                 target.z + distance_ * std::cos(pitch_) * std::cos(yaw_)};

    float view[16], proj[16];
    lookAt(eye, target, {0.0f, 1.0f, 0.0f}, view);
    const float aspect = fbH > 0 ? static_cast<float>(fbW) / static_cast<float>(fbH) : 1.0f;
    perspective(45.0f * kPi / 180.0f, aspect, distance_ * 0.01f, distance_ * 100.0f, proj);
    multiply(proj, view, viewProj_);

    glUseProgram(program_);
    glUniformMatrix4fv(uViewProj_, 1, GL_FALSE, viewProj_);
}

void Renderer::endFrame() { glfwSwapBuffers(window_); }

void Renderer::drawPoints(const void* firstPosition, std::size_t count, std::size_t strideBytes,
                          Color color, float sizePixels) {
    draw(GL_POINTS, firstPosition, count, strideBytes, color, sizePixels, true);
}

void Renderer::drawLineStrip(const void* firstPosition, std::size_t count,
                             std::size_t strideBytes, Color color) {
    draw(GL_LINE_STRIP, firstPosition, count, strideBytes, color, 1.0f, false);
}

void Renderer::drawGrid(float halfSize, int divisions, Color color) {
    std::vector<float> lines;
    lines.reserve(static_cast<std::size_t>(divisions + 1) * 12);
    const float step = 2.0f * halfSize / static_cast<float>(divisions);
    for (int i = 0; i <= divisions; ++i) {
        const float t = -halfSize + step * static_cast<float>(i);
        lines.insert(lines.end(), {t, 0.0f, -halfSize, t, 0.0f, halfSize});
        lines.insert(lines.end(), {-halfSize, 0.0f, t, halfSize, 0.0f, t});
    }
    draw(GL_LINES, lines.data(), lines.size() / 3, 3 * sizeof(float), color, 1.0f, false);
}

void Renderer::draw(unsigned mode, const void* data, std::size_t count, std::size_t strideBytes,
                    Color color, float pointSize, bool roundPoints) {
    if (!data || count == 0) return;
    if (strideBytes == 0) strideBytes = 3 * sizeof(float);

    // Upload exactly the bytes spanned by the strided positions.
    const std::size_t bytes = (count - 1) * strideBytes + 3 * sizeof(float);
    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(bytes), data, GL_STREAM_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, static_cast<GLsizei>(strideBytes), nullptr);

    glUniform3f(uColor_, color.r, color.g, color.b);
    glUniform1f(uPointSize_, pointSize * pixelScale_);
    glUniform1i(uRoundPoints_, roundPoints ? 1 : 0);
    glDrawArrays(mode, 0, static_cast<GLsizei>(count));
}

void Renderer::setCameraTarget(float x, float y, float z) {
    target_[0] = x;
    target_[1] = y;
    target_[2] = z;
}

void Renderer::setCameraDistance(float distance) { distance_ = std::max(distance, 1e-3f); }

bool Renderer::isKeyDown(int glfwKey) const { return glfwGetKey(window_, glfwKey) == GLFW_PRESS; }

double Renderer::time() const { return glfwGetTime(); }

// ---- Input callbacks --------------------------------------------------------
void Renderer::cursorPosCallback(GLFWwindow* w, double x, double y) {
    auto* self = static_cast<Renderer*>(glfwGetWindowUserPointer(w));
    if (self->haveLastCursor_ && glfwGetMouseButton(w, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
        constexpr float kRadiansPerPixel = 0.008f;
        self->yaw_ -= static_cast<float>(x - self->lastX_) * kRadiansPerPixel;
        self->pitch_ += static_cast<float>(y - self->lastY_) * kRadiansPerPixel;
        const float limit = 89.0f * kPi / 180.0f;
        self->pitch_ = std::clamp(self->pitch_, -limit, limit);
    }
    self->lastX_ = x;
    self->lastY_ = y;
    self->haveLastCursor_ = true;
}

void Renderer::scrollCallback(GLFWwindow* w, double /*dx*/, double dy) {
    auto* self = static_cast<Renderer*>(glfwGetWindowUserPointer(w));
    self->distance_ *= std::pow(0.9f, static_cast<float>(dy));
    self->distance_ = std::max(self->distance_, 1e-3f);
}

void Renderer::keyCallback(GLFWwindow* w, int key, int /*scancode*/, int action, int /*mods*/) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) glfwSetWindowShouldClose(w, GLFW_TRUE);
}

}  // namespace render
