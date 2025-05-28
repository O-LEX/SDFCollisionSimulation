#include "sdf.h"
#include <iostream>
#include <limits>
#include <cmath>

SDF::SDF(int resolution) : resolution(resolution) {
    data.resize(resolution * resolution * resolution);
}

void SDF::generateFromMesh(const Mesh& mesh) {
    minBounds = mesh.getMin();
    maxBounds = mesh.getMax();
    
    // Add padding
    glm::vec3 padding = (maxBounds - minBounds) * 0.1f;
    minBounds -= padding;
    maxBounds += padding;
    
    cellSize = (maxBounds - minBounds) / float(resolution - 1);
    
    std::cout << "Generating SDF with resolution " << resolution << "x" << resolution << "x" << resolution << std::endl;
    std::cout << "Bounds: (" << minBounds.x << "," << minBounds.y << "," << minBounds.z << ") to ("
              << maxBounds.x << "," << maxBounds.y << "," << maxBounds.z << ")" << std::endl;
    
    // Build BVH for acceleration
    bvh.build(mesh.getTriangles());
    
    const auto& triangles = mesh.getTriangles();
    
    for (int z = 0; z < resolution; ++z) {
        if (z % 10 == 0) {
            std::cout << "Progress: " << (100 * z / resolution) << "%" << std::endl;
        }
        
        for (int y = 0; y < resolution; ++y) {
            for (int x = 0; x < resolution; ++x) {
                glm::vec3 worldPos = minBounds + glm::vec3(x, y, z) * cellSize;
                
                // Use BVH for fast distance calculation
                float minDistance = bvh.findClosestDistance(worldPos, triangles);
                
                // Determine sign (inside/outside) using BVH accelerated ray casting
                glm::vec3 rayDir(1, 0, 0);
                int intersections = bvh.countIntersections(worldPos, rayDir, triangles);
                bool inside = (intersections % 2) == 1;
                
                if (inside) {
                    minDistance = -minDistance;
                }
                
                data[getIndex(x, y, z)] = minDistance;
            }
        }
    }
    
    std::cout << "SDF generation complete" << std::endl;
}

float SDF::sample(const glm::vec3& position) const {
    glm::vec3 gridPos = worldToGrid(position);
    
    // Clamp to grid bounds
    gridPos = glm::clamp(gridPos, glm::vec3(0), glm::vec3(resolution - 1));
    
    // Trilinear interpolation
    int x0 = (int)gridPos.x;
    int y0 = (int)gridPos.y;
    int z0 = (int)gridPos.z;
    int x1 = std::min(x0 + 1, resolution - 1);
    int y1 = std::min(y0 + 1, resolution - 1);
    int z1 = std::min(z0 + 1, resolution - 1);
    
    float fx = gridPos.x - x0;
    float fy = gridPos.y - y0;
    float fz = gridPos.z - z0;
    
    float c000 = data[getIndex(x0, y0, z0)];
    float c001 = data[getIndex(x0, y0, z1)];
    float c010 = data[getIndex(x0, y1, z0)];
    float c011 = data[getIndex(x0, y1, z1)];
    float c100 = data[getIndex(x1, y0, z0)];
    float c101 = data[getIndex(x1, y0, z1)];
    float c110 = data[getIndex(x1, y1, z0)];
    float c111 = data[getIndex(x1, y1, z1)];
    
    float c00 = c000 * (1 - fx) + c100 * fx;
    float c01 = c001 * (1 - fx) + c101 * fx;
    float c10 = c010 * (1 - fx) + c110 * fx;
    float c11 = c011 * (1 - fx) + c111 * fx;
    
    float c0 = c00 * (1 - fy) + c10 * fy;
    float c1 = c01 * (1 - fy) + c11 * fy;
    
    return c0 * (1 - fz) + c1 * fz;
}

glm::vec3 SDF::gradient(const glm::vec3& position) const {
    const float epsilon = cellSize.x * 0.1f;
    
    float dx = sample(position + glm::vec3(epsilon, 0, 0)) - sample(position - glm::vec3(epsilon, 0, 0));
    float dy = sample(position + glm::vec3(0, epsilon, 0)) - sample(position - glm::vec3(0, epsilon, 0));
    float dz = sample(position + glm::vec3(0, 0, epsilon)) - sample(position - glm::vec3(0, 0, epsilon));
    
    return glm::vec3(dx, dy, dz) / (2.0f * epsilon);
}

float SDF::pointToTriangleDistance(const glm::vec3& point, const Triangle& triangle) const {
    glm::vec3 edge0 = triangle.v1 - triangle.v0;
    glm::vec3 edge1 = triangle.v2 - triangle.v0;
    glm::vec3 v0 = triangle.v0 - point;
    
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
    
    glm::vec3 closest = triangle.v0 + s * edge0 + t * edge1;
    return glm::length(point - closest);
}

glm::vec3 SDF::worldToGrid(const glm::vec3& worldPos) const {
    return (worldPos - minBounds) / cellSize;
}

int SDF::getIndex(int x, int y, int z) const {
    return z * resolution * resolution + y * resolution + x;
}
