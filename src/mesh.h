#pragma once

#include <vector>
#include <string>
#include <glm/glm.hpp>
#include <GL/glew.h> // Added for GLuint

struct Triangle {
    glm::vec3 v0, v1, v2;
    glm::vec3 normal;
};

class Mesh {
public:
    Mesh(); // Constructor
    ~Mesh(); // Destructor

    bool loadOBJ(const std::string& filename);
    const std::vector<Triangle>& getTriangles() const { return triangles; }
    glm::vec3 getMin() const { return minBounds; }
    glm::vec3 getMax() const { return maxBounds; }

    void draw() const; // Drawing method
    
private:
    void setupMesh(); // Helper to setup VAO/VBO/EBO
    std::vector<Triangle> triangles; // For SDF
    glm::vec3 minBounds, maxBounds;
    
    // For rendering
    std::vector<glm::vec3> vertices_for_rendering;
    std::vector<unsigned int> indices_for_rendering;
    GLuint VAO, VBO, EBO;
    bool meshSetupDone;

    void computeBounds();
    glm::vec3 computeTriangleNormal(const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2);
};
