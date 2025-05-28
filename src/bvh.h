#pragma once

#include <vector>
#include <memory>
#include <glm/glm.hpp>

struct Triangle;

struct BVHNode {
    glm::vec3 minBounds, maxBounds;
    std::vector<int> triangleIndices;
    std::unique_ptr<BVHNode> left, right;
    bool isLeaf;
    
    BVHNode() : isLeaf(false) {}
};

class BVH {
public:
    void build(const std::vector<Triangle>& triangles);
    float findClosestDistance(const glm::vec3& point, const std::vector<Triangle>& triangles) const;
    int countIntersections(const glm::vec3& point, const glm::vec3& direction, const std::vector<Triangle>& triangles) const;
    
private:
    std::unique_ptr<BVHNode> root;
    
    std::unique_ptr<BVHNode> buildRecursive(const std::vector<Triangle>& triangles, std::vector<int>& indices, int depth = 0);
    glm::vec3 computeCentroid(const Triangle& triangle) const;
    float pointToAABBDistance(const glm::vec3& point, const glm::vec3& minBounds, const glm::vec3& maxBounds) const;
    float findClosestDistanceRecursive(const glm::vec3& point, const std::vector<Triangle>& triangles, const BVHNode* node, float& bestDistance) const;
    int countIntersectionsRecursive(const glm::vec3& point, const glm::vec3& direction, const std::vector<Triangle>& triangles, const BVHNode* node) const;
    bool rayAABBIntersect(const glm::vec3& origin, const glm::vec3& direction, const glm::vec3& minBounds, const glm::vec3& maxBounds) const;
};
