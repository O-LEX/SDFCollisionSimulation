#pragma once

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include "particle.h"
#include "mesh.h"

class Renderer {
public:
    Renderer(int width, int height);
    ~Renderer();
    
    bool initialize();
    void cleanup();
    
    void beginFrame();
    void endFrame();
    
    void drawWireframeBox(const glm::vec3& min, const glm::vec3& max);
    void drawParticles(const std::vector<Particle>& particles);
    void drawMesh(const Mesh& mesh, const glm::vec3& position);
    void drawMeshes(const std::vector<const Mesh*>& meshes, const std::vector<glm::vec3>& positions);
    void setCamera(const glm::vec3& position, const glm::vec3& target, const glm::vec3& up);
    void setPerspective(float fov, float aspect, float near, float far);
    
    // Orbit camera controls
    void updateCamera();
    void handleMouseInput();
    
    GLFWwindow* getWindow() const { return window; }
    bool shouldClose() const { return glfwWindowShouldClose(window); }
    
private:
    GLFWwindow* window;
    int width, height;
    
    GLuint shaderProgram;
    GLuint boxVAO, boxVBO, boxEBO;
    GLuint sphereVAO, sphereVBO, sphereEBO;
    GLuint meshVAO, meshVBO;
    int sphereIndexCount;
    
    glm::mat4 viewMatrix;
    glm::mat4 projectionMatrix;
    
    // Orbit camera parameters
    glm::vec3 target;
    float radius;
    float theta;  // horizontal angle
    float phi;    // vertical angle
    
    // Mouse input state
    bool mousePressed;
    double lastMouseX, lastMouseY;
    
    bool createShaderProgram();
    void setupBoxGeometry();
    void setupSphereGeometry();
    GLuint compileShader(const char* source, GLenum type);
    
    // Camera callbacks
    static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
    static void cursorPosCallback(GLFWwindow* window, double xpos, double ypos);
    static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);
};
