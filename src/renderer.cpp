#include "renderer.h"
#include <iostream>
#include <cmath>

// Vertex shader source
const char* vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec3 aPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}
)";

// Fragment shader source
const char* fragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;

uniform vec3 color;

void main() {
    FragColor = vec4(color, 1.0);
}
)";

Renderer::Renderer(int width, int height) : width(width), height(height), window(nullptr) {
    viewMatrix = glm::mat4(1.0f);
    projectionMatrix = glm::mat4(1.0f);
    
    // Initialize orbit camera parameters
    target = glm::vec3(0.0f, 0.0f, 0.0f);
    radius = 5.0f;
    theta = 0.0f;
    phi = 0.3f;  // slightly above horizontal
    
    // Initialize mouse state
    mousePressed = false;
    lastMouseX = 0.0;
    lastMouseY = 0.0;
}

Renderer::~Renderer() {
    cleanup();
}

bool Renderer::initialize() {
    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return false;
    }
    
    // Configure GLFW
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
    // Create window
    window = glfwCreateWindow(width, height, "SDF Collision Simulation", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }
    
    glfwMakeContextCurrent(window);
    
    // Initialize GLEW
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW" << std::endl;
        return false;
    }
    
    // Set viewport
    glViewport(0, 0, width, height);
    
    // Enable depth testing
    glEnable(GL_DEPTH_TEST);
    
    // Create shader program
    if (!createShaderProgram()) {
        return false;
    }
    
    // Setup box geometry
    setupBoxGeometry();
    
    // Setup sphere geometry
    setupSphereGeometry();
    
    // Setup camera callbacks
    glfwSetWindowUserPointer(window, this);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetCursorPosCallback(window, cursorPosCallback);
    glfwSetScrollCallback(window, scrollCallback);
    
    // Set default camera
    updateCamera();
    setPerspective(45.0f, (float)width / height, 0.1f, 100.0f);
    
    return true;
}

void Renderer::cleanup() {
    if (boxVAO) glDeleteVertexArrays(1, &boxVAO);
    if (boxVBO) glDeleteBuffers(1, &boxVBO);
    if (boxEBO) glDeleteBuffers(1, &boxEBO);
    if (sphereVAO) glDeleteVertexArrays(1, &sphereVAO);
    if (sphereVBO) glDeleteBuffers(1, &sphereVBO);
    if (sphereEBO) glDeleteBuffers(1, &sphereEBO);
    if (meshVAO) glDeleteVertexArrays(1, &meshVAO);
    if (meshVBO) glDeleteBuffers(1, &meshVBO);
    if (shaderProgram) glDeleteProgram(shaderProgram);
    
    if (window) {
        glfwDestroyWindow(window);
        window = nullptr;
    }
    glfwTerminate();
}

void Renderer::beginFrame() {
    handleMouseInput();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
}

void Renderer::endFrame() {
    glfwSwapBuffers(window);
    glfwPollEvents();
}

void Renderer::drawWireframeBox(const glm::vec3& min, const glm::vec3& max) {
    glUseProgram(shaderProgram);
    
    // Create transformation matrix to scale and translate the unit box
    glm::vec3 center = (min + max) * 0.5f;
    glm::vec3 size = max - min;
    
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, center);
    model = glm::scale(model, size);
    
    // Set uniforms
    GLint modelLoc = glGetUniformLocation(shaderProgram, "model");
    GLint viewLoc = glGetUniformLocation(shaderProgram, "view");
    GLint projLoc = glGetUniformLocation(shaderProgram, "projection");
    GLint colorLoc = glGetUniformLocation(shaderProgram, "color");
    
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(viewMatrix));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projectionMatrix));
    glUniform3f(colorLoc, 1.0f, 1.0f, 1.0f); // White color
    
    // Draw wireframe
    glBindVertexArray(boxVAO);
    glDrawElements(GL_LINES, 24, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void Renderer::setCamera(const glm::vec3& position, const glm::vec3& target, const glm::vec3& up) {
    viewMatrix = glm::lookAt(position, target, up);
}

void Renderer::setPerspective(float fov, float aspect, float near, float far) {
    projectionMatrix = glm::perspective(glm::radians(fov), aspect, near, far);
}

bool Renderer::createShaderProgram() {
    GLuint vertexShader = compileShader(vertexShaderSource, GL_VERTEX_SHADER);
    GLuint fragmentShader = compileShader(fragmentShaderSource, GL_FRAGMENT_SHADER);
    
    if (vertexShader == 0 || fragmentShader == 0) {
        return false;
    }
    
    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    
    GLint success;
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(shaderProgram, 512, nullptr, infoLog);
        std::cerr << "Shader program linking failed: " << infoLog << std::endl;
        return false;
    }
    
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    
    return true;
}

void Renderer::setupBoxGeometry() {
    // Unit cube vertices (centered at origin)
    float vertices[] = {
        // Front face
        -0.5f, -0.5f,  0.5f,  // 0
         0.5f, -0.5f,  0.5f,  // 1
         0.5f,  0.5f,  0.5f,  // 2
        -0.5f,  0.5f,  0.5f,  // 3
        // Back face
        -0.5f, -0.5f, -0.5f,  // 4
         0.5f, -0.5f, -0.5f,  // 5
         0.5f,  0.5f, -0.5f,  // 6
        -0.5f,  0.5f, -0.5f   // 7
    };
    
    // Indices for drawing only the edges (12 edges total)
    unsigned int indices[] = {
        // Front face edges
        0, 1,  1, 2,  2, 3,  3, 0,
        // Back face edges
        4, 5,  5, 6,  6, 7,  7, 4,
        // Connecting edges between front and back
        0, 4,  1, 5,  2, 6,  3, 7
    };
    
    glGenVertexArrays(1, &boxVAO);
    glGenBuffers(1, &boxVBO);
    glGenBuffers(1, &boxEBO);
    
    glBindVertexArray(boxVAO);
    
    glBindBuffer(GL_ARRAY_BUFFER, boxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, boxEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    glBindVertexArray(0);
}

GLuint Renderer::compileShader(const char* source, GLenum type) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        std::cerr << "Shader compilation failed: " << infoLog << std::endl;
        return 0;
    }
    
    return shader;
}

void Renderer::updateCamera() {
    // Convert spherical coordinates to Cartesian
    float x = radius * cos(phi) * cos(theta);
    float y = radius * sin(phi);
    float z = radius * cos(phi) * sin(theta);
    
    glm::vec3 position = target + glm::vec3(x, y, z);
    viewMatrix = glm::lookAt(position, target, glm::vec3(0.0f, 1.0f, 0.0f));
}

void Renderer::handleMouseInput() {
    // This method is called every frame to handle continuous input
    // The actual input processing is done in the callbacks
}

void Renderer::drawParticles(const std::vector<Particle>& particles) {
    glUseProgram(shaderProgram);
    
    // Get uniform locations
    GLint modelLoc = glGetUniformLocation(shaderProgram, "model");
    GLint viewLoc = glGetUniformLocation(shaderProgram, "view");
    GLint projLoc = glGetUniformLocation(shaderProgram, "projection");
    GLint colorLoc = glGetUniformLocation(shaderProgram, "color");
    
    // Set view and projection matrices
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(viewMatrix));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projectionMatrix));
    
    // Set particle color (red)
    glUniform3f(colorLoc, 1.0f, 0.3f, 0.3f);
    
    glBindVertexArray(sphereVAO);
    
    // Draw each particle
    for (const auto& particle : particles) {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, particle.getPosition());
        model = glm::scale(model, glm::vec3(particle.getSize()));
        
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glDrawElements(GL_TRIANGLES, sphereIndexCount, GL_UNSIGNED_INT, 0);
    }
    
    glBindVertexArray(0);
}

void Renderer::setupSphereGeometry() {
    // Create a simple icosphere (subdivided icosahedron)
    std::vector<glm::vec3> vertices;
    std::vector<unsigned int> indices;
    
    // Golden ratio
    const float phi = (1.0f + sqrt(5.0f)) / 2.0f;
    const float a = 1.0f;
    const float b = 1.0f / phi;
    
    // 12 vertices of icosahedron
    vertices = {
        {0, b, -a}, {b, a, 0}, {-b, a, 0}, {0, b, a},
        {0, -b, a}, {-a, 0, b}, {0, -b, -a}, {a, 0, -b},
        {a, 0, b}, {-a, 0, -b}, {b, -a, 0}, {-b, -a, 0}
    };
    
    // Normalize vertices to unit sphere
    for (auto& vertex : vertices) {
        vertex = glm::normalize(vertex);
    }
    
    // 20 triangular faces
    indices = {
        2, 1, 0,   1, 2, 3,   5, 4, 3,   4, 8, 3,
        7, 6, 0,   6, 9, 0,   11, 10, 4, 10, 11, 6,
        9, 5, 2,   5, 9, 11,  8, 7, 1,   7, 8, 10,
        2, 5, 3,   8, 1, 3,   9, 2, 0,   1, 7, 0,
        11, 9, 6,  7, 10, 6,  5, 11, 4,  10, 8, 4
    };
    
    sphereIndexCount = indices.size();
    
    // Convert vertices to float array
    std::vector<float> vertexData;
    for (const auto& vertex : vertices) {
        vertexData.push_back(vertex.x);
        vertexData.push_back(vertex.y);
        vertexData.push_back(vertex.z);
    }
    
    glGenVertexArrays(1, &sphereVAO);
    glGenBuffers(1, &sphereVBO);
    glGenBuffers(1, &sphereEBO);
    
    glBindVertexArray(sphereVAO);
    
    glBindBuffer(GL_ARRAY_BUFFER, sphereVBO);
    glBufferData(GL_ARRAY_BUFFER, vertexData.size() * sizeof(float), vertexData.data(), GL_STATIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sphereEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    glBindVertexArray(0);
}

void Renderer::drawMesh(const Mesh& mesh, const glm::vec3& position) {
    glUseProgram(shaderProgram);
    
    // Create transformation matrix
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, position);
    
    // Set uniforms
    GLint modelLoc = glGetUniformLocation(shaderProgram, "model");
    GLint viewLoc = glGetUniformLocation(shaderProgram, "view");
    GLint projLoc = glGetUniformLocation(shaderProgram, "projection");
    GLint colorLoc = glGetUniformLocation(shaderProgram, "color");
    
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(viewMatrix));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projectionMatrix));
    glUniform3f(colorLoc, 0.3f, 0.8f, 0.3f); // Green color for mesh
    
    // Generate VAO and VBO for this mesh if not already done
    if (meshVAO == 0) {
        glGenVertexArrays(1, &meshVAO);
        glGenBuffers(1, &meshVBO);
    }
    
    // Upload mesh data from triangles
    const auto& triangles = mesh.getTriangles();
    std::vector<float> vertexData;
    for (const auto& triangle : triangles) {
        // Add vertices for each triangle
        vertexData.push_back(triangle.v0.x);
        vertexData.push_back(triangle.v0.y);
        vertexData.push_back(triangle.v0.z);
        
        vertexData.push_back(triangle.v1.x);
        vertexData.push_back(triangle.v1.y);
        vertexData.push_back(triangle.v1.z);
        
        vertexData.push_back(triangle.v2.x);
        vertexData.push_back(triangle.v2.y);
        vertexData.push_back(triangle.v2.z);
    }
    
    glBindVertexArray(meshVAO);
    glBindBuffer(GL_ARRAY_BUFFER, meshVBO);
    glBufferData(GL_ARRAY_BUFFER, vertexData.size() * sizeof(float), vertexData.data(), GL_DYNAMIC_DRAW);
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // Draw as wireframe
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glDrawArrays(GL_TRIANGLES, 0, triangles.size() * 3);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    
    glBindVertexArray(0);
}

// Static callback functions
void Renderer::mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    Renderer* renderer = static_cast<Renderer*>(glfwGetWindowUserPointer(window));
    
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            renderer->mousePressed = true;
            glfwGetCursorPos(window, &renderer->lastMouseX, &renderer->lastMouseY);
        } else if (action == GLFW_RELEASE) {
            renderer->mousePressed = false;
        }
    }
}

void Renderer::cursorPosCallback(GLFWwindow* window, double xpos, double ypos) {
    Renderer* renderer = static_cast<Renderer*>(glfwGetWindowUserPointer(window));
    
    if (renderer->mousePressed) {
        double deltaX = xpos - renderer->lastMouseX;
        double deltaY = ypos - renderer->lastMouseY;
        
        // Update angles based on mouse movement
        renderer->theta += deltaX * 0.01f;  // horizontal rotation
        renderer->phi += deltaY * 0.01f;    // vertical rotation (inverted for intuitive control)
        
        // Clamp phi to prevent camera flipping
        const float maxPhi = 1.5f;
        if (renderer->phi > maxPhi) renderer->phi = maxPhi;
        if (renderer->phi < -maxPhi) renderer->phi = -maxPhi;
        
        // Update camera
        renderer->updateCamera();
        
        renderer->lastMouseX = xpos;
        renderer->lastMouseY = ypos;
    }
}

void Renderer::scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
    Renderer* renderer = static_cast<Renderer*>(glfwGetWindowUserPointer(window));
    
    // Zoom in/out by changing radius
    renderer->radius -= yoffset * 0.5f;
    
    // Clamp radius to reasonable bounds
    if (renderer->radius < 0.1f) renderer->radius = 0.1f;
    if (renderer->radius > 20.0f) renderer->radius = 20.0f;
    
    // Update camera
    renderer->updateCamera();
}
