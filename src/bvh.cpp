#include "bvh.h"
#include "mesh.h"
#include <algorithm>
#include <iostream>
#include <limits>

void BVH::build(const std::vector<Triangle>& triangles) {
    std::cout << "Building BVH for " << triangles.size() << " triangles..." << std::endl;
    
    std::vector<int> indices(triangles.size());
    for (int i = 0; i < triangles.size(); ++i) {
        indices[i] = i;
    }
    
    root = buildRecursive(triangles, indices);
    std::cout << "BVH construction complete" << std::endl;
}

std::unique_ptr<BVHNode> BVH::buildRecursive(const std::vector<Triangle>& triangles, std::vector<int>& indices, int depth) {
    auto node = std::make_unique<BVHNode>();
    
    // Compute bounding box
    node->minBounds = glm::vec3(std::numeric_limits<float>::max());
    node->maxBounds = glm::vec3(std::numeric_limits<float>::lowest());
    
    for (int idx : indices) {
        const Triangle& tri = triangles[idx];
        for (const auto& vertex : {tri.v0, tri.v1, tri.v2}) {
            node->minBounds = glm::min(node->minBounds, vertex);
            node->maxBounds = glm::max(node->maxBounds, vertex);
        }
    }
    
    // Leaf node condition
    if (indices.size() <= 4 || depth > 20) {
        node->isLeaf = true;
        node->triangleIndices = indices;
        return node;
    }
    
    // Find longest axis
    glm::vec3 extent = node->maxBounds - node->minBounds;
    int axis = 0;
    if (extent.y > extent.x) axis = 1;
    if (extent.z > extent[axis]) axis = 2;
    
    // Sort triangles by centroid along the chosen axis
    std::sort(indices.begin(), indices.end(), [&](int a, int b) {
        glm::vec3 centroidA = computeCentroid(triangles[a]);
        glm::vec3 centroidB = computeCentroid(triangles[b]);
        return centroidA[axis] < centroidB[axis];
    });
    
    // Split in the middle
    int mid = indices.size() / 2;
    std::vector<int> leftIndices(indices.begin(), indices.begin() + mid);
    std::vector<int> rightIndices(indices.begin() + mid, indices.end());
    
    node->left = buildRecursive(triangles, leftIndices, depth + 1);
    node->right = buildRecursive(triangles, rightIndices, depth + 1);
    
    return node;
}

glm::vec3 BVH::computeCentroid(const Triangle& triangle) const {
    return (triangle.v0 + triangle.v1 + triangle.v2) / 3.0f;
}

float BVH::findClosestDistance(const glm::vec3& point, const std::vector<Triangle>& triangles) const {
    if (!root) return std::numeric_limits<float>::max();
    float bestDistance = std::numeric_limits<float>::max();
    return findClosestDistanceRecursive(point, triangles, root.get(), bestDistance);
}

float BVH::findClosestDistanceRecursive(const glm::vec3& point, const std::vector<Triangle>& triangles, const BVHNode* node, float& bestDistance) const {
    if (!node) return std::numeric_limits<float>::max();
    
    // Early termination if point is too far from AABB
    float aabbDist = pointToAABBDistance(point, node->minBounds, node->maxBounds);
    if (aabbDist >= bestDistance) {
        return std::numeric_limits<float>::max();
    }
    
    if (node->isLeaf) {
        float minDist = bestDistance;
        for (int idx : node->triangleIndices) {
            // Use the same point-to-triangle distance function from SDF class
            const Triangle& tri = triangles[idx];
            
            // Quick bounding sphere check for early rejection
            glm::vec3 center = (tri.v0 + tri.v1 + tri.v2) / 3.0f;
            float maxEdge = std::max({
                glm::length(tri.v1 - tri.v0),
                glm::length(tri.v2 - tri.v1),
                glm::length(tri.v0 - tri.v2)
            });
            float sphereRadius = maxEdge * 0.6f; // Conservative estimate
            float sphereDist = glm::length(point - center) - sphereRadius;
            
            if (sphereDist >= minDist) continue;
            
            glm::vec3 edge0 = tri.v1 - tri.v0;
            glm::vec3 edge1 = tri.v2 - tri.v0;
            glm::vec3 v0 = tri.v0 - point;
            
            float a = glm::dot(edge0, edge0);
            float b = glm::dot(edge0, edge1);
            float c = glm::dot(edge1, edge1);
            float d = glm::dot(edge0, v0);
            float e = glm::dot(edge1, v0);
            
            float det = a * c - b * b;
            float s = b * e - c * d;
            float t = b * d - a * e;
            
            if (s + t < det) {
                if (s < 0) {
                    if (t < 0) {
                        if (d < 0) {
                            s = glm::clamp(-d / a, 0.0f, 1.0f);
                            t = 0;
                        } else {
                            s = 0;
                            t = glm::clamp(-e / c, 0.0f, 1.0f);
                        }
                    } else {
                        s = 0;
                        t = glm::clamp(-e / c, 0.0f, 1.0f);
                    }
                } else if (t < 0) {
                    s = glm::clamp(-d / a, 0.0f, 1.0f);
                    t = 0;
                } else {
                    float invDet = 1 / det;
                    s *= invDet;
                    t *= invDet;
                }
            } else {
                if (s < 0) {
                    float tmp0 = b + d;
                    float tmp1 = c + e;
                    if (tmp1 > tmp0) {
                        float numer = tmp1 - tmp0;
                        float denom = a - 2 * b + c;
                        s = glm::clamp(numer / denom, 0.0f, 1.0f);
                        t = 1 - s;
                    } else {
                        t = glm::clamp(-e / c, 0.0f, 1.0f);
                        s = 0;
                    }
                } else if (t < 0) {
                    if (a + d > b + e) {
                        float numer = c + e - b - d;
                        float denom = a - 2 * b + c;
                        s = glm::clamp(numer / denom, 0.0f, 1.0f);
                        t = 1 - s;
                    } else {
                        s = glm::clamp(-d / a, 0.0f, 1.0f);
                        t = 0;
                    }
                } else {
                    float numer = c + e - b - d;
                    float denom = a - 2 * b + c;
                    s = glm::clamp(numer / denom, 0.0f, 1.0f);
                    t = 1 - s;
                }
            }
            
            glm::vec3 closest = tri.v0 + s * edge0 + t * edge1;
            float distance = glm::length(point - closest);
            minDist = std::min(minDist, distance);
        }
        bestDistance = std::min(bestDistance, minDist);
        return minDist;
    }
    
    // Sort children by distance to prioritize closer nodes
    float leftAABBDist = pointToAABBDistance(point, node->left->minBounds, node->left->maxBounds);
    float rightAABBDist = pointToAABBDistance(point, node->right->minBounds, node->right->maxBounds);
    
    float leftDist = std::numeric_limits<float>::max();
    float rightDist = std::numeric_limits<float>::max();
    
    if (leftAABBDist <= rightAABBDist) {
        if (leftAABBDist < bestDistance) {
            leftDist = findClosestDistanceRecursive(point, triangles, node->left.get(), bestDistance);
        }
        if (rightAABBDist < bestDistance) {
            rightDist = findClosestDistanceRecursive(point, triangles, node->right.get(), bestDistance);
        }
    } else {
        if (rightAABBDist < bestDistance) {
            rightDist = findClosestDistanceRecursive(point, triangles, node->right.get(), bestDistance);
        }
        if (leftAABBDist < bestDistance) {
            leftDist = findClosestDistanceRecursive(point, triangles, node->left.get(), bestDistance);
        }
    }
    
    return std::min(leftDist, rightDist);
}

float BVH::pointToAABBDistance(const glm::vec3& point, const glm::vec3& minBounds, const glm::vec3& maxBounds) const {
    glm::vec3 closest = glm::clamp(point, minBounds, maxBounds);
    return glm::length(point - closest);
}

int BVH::countIntersections(const glm::vec3& point, const glm::vec3& direction, const std::vector<Triangle>& triangles) const {
    if (!root) return 0;
    return countIntersectionsRecursive(point, direction, triangles, root.get());
}

int BVH::countIntersectionsRecursive(const glm::vec3& point, const glm::vec3& direction, const std::vector<Triangle>& triangles, const BVHNode* node) const {
    if (!node) return 0;
    
    // Check if ray intersects AABB
    if (!rayAABBIntersect(point, direction, node->minBounds, node->maxBounds)) {
        return 0;
    }
    
    if (node->isLeaf) {
        int intersections = 0;
        for (int idx : node->triangleIndices) {
            const Triangle& tri = triangles[idx];
            
            glm::vec3 edge1 = tri.v1 - tri.v0;
            glm::vec3 edge2 = tri.v2 - tri.v0;
            glm::vec3 h = glm::cross(direction, edge2);
            float a = glm::dot(edge1, h);
            
            if (a > -1e-7 && a < 1e-7) continue;
            
            float f = 1.0f / a;
            glm::vec3 s = point - tri.v0;
            float u = f * glm::dot(s, h);
            
            if (u < 0.0f || u > 1.0f) continue;
            
            glm::vec3 q = glm::cross(s, edge1);
            float v = f * glm::dot(direction, q);
            
            if (v < 0.0f || u + v > 1.0f) continue;
            
            float t = f * glm::dot(edge2, q);
            if (t > 1e-7) {
                intersections++;
            }
        }
        return intersections;
    }
    
    int leftIntersections = countIntersectionsRecursive(point, direction, triangles, node->left.get());
    int rightIntersections = countIntersectionsRecursive(point, direction, triangles, node->right.get());
    
    return leftIntersections + rightIntersections;
}

bool BVH::rayAABBIntersect(const glm::vec3& origin, const glm::vec3& direction, const glm::vec3& minBounds, const glm::vec3& maxBounds) const {
    glm::vec3 invDir = 1.0f / direction;
    glm::vec3 t1 = (minBounds - origin) * invDir;
    glm::vec3 t2 = (maxBounds - origin) * invDir;
    
    glm::vec3 tmin = glm::min(t1, t2);
    glm::vec3 tmax = glm::max(t1, t2);
    
    float tNear = std::max({tmin.x, tmin.y, tmin.z});
    float tFar = std::min({tmax.x, tmax.y, tmax.z});
    
    return tNear <= tFar && tFar >= 0;
}
