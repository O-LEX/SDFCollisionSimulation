#pragma once

#include "mesh.h"
#include "bvh.h"
#include <glm/glm.hpp>
#include <vector>

class SDF {
public:
    SDF(int resolution);
    
    void generateFromMesh(const Mesh& mesh);
    float sample(const glm::vec3& position) const;
    glm::vec3 gradient(const glm::vec3& position) const;
    
    int getResolution() const { return resolution; }
    glm::vec3 getMin() const { return minBounds; }
    glm::vec3 getMax() const { return maxBounds; }
    
private:
    int resolution;
    std::vector<float> data;
    glm::vec3 minBounds, maxBounds;
    glm::vec3 cellSize;
    BVH bvh;
    
    float pointToTriangleDistance(const glm::vec3& point, const Triangle& triangle) const;
    glm::vec3 worldToGrid(const glm::vec3& worldPos) const;
    int getIndex(int x, int y, int z) const;
};
