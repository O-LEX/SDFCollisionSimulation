#pragma once

#include <vector>
#include <string>
#include <glm/glm.hpp>

struct Triangle {
    glm::vec3 v0, v1, v2;
    glm::vec3 normal;
};

class Mesh {
public:
    bool loadOBJ(const std::string& filename);
    const std::vector<Triangle>& getTriangles() const { return triangles; }
    glm::vec3 getMin() const { return minBounds; }
    glm::vec3 getMax() const { return maxBounds; }
    
private:
    std::vector<Triangle> triangles;
    glm::vec3 minBounds, maxBounds;
    
    void computeBounds();
    glm::vec3 computeTriangleNormal(const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2);
};
