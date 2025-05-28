#include "mesh.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <limits>

bool Mesh::loadOBJ(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Failed to open OBJ file: " << filename << std::endl;
        return false;
    }
    
    std::vector<glm::vec3> vertices;
    std::vector<std::vector<int>> faces;
    
    std::string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string prefix;
        iss >> prefix;
        
        if (prefix == "v") {
            float x, y, z;
            iss >> x >> y >> z;
            vertices.push_back(glm::vec3(x, y, z));
        }
        else if (prefix == "f") {
            std::vector<int> face;
            std::string vertex;
            while (iss >> vertex) {
                // Handle "vertex/texture/normal" format
                size_t slash = vertex.find('/');
                int vertexIndex = std::stoi(vertex.substr(0, slash)) - 1; // OBJ is 1-indexed
                face.push_back(vertexIndex);
            }
            if (face.size() >= 3) {
                faces.push_back(face);
            }
        }
    }
    
    // Convert faces to triangles
    triangles.clear();
    for (const auto& face : faces) {
        // Triangulate if face has more than 3 vertices
        for (size_t i = 1; i < face.size() - 1; ++i) {
            Triangle tri;
            tri.v0 = vertices[face[0]];
            tri.v1 = vertices[face[i]];
            tri.v2 = vertices[face[i + 1]];
            tri.normal = computeTriangleNormal(tri.v0, tri.v1, tri.v2);
            triangles.push_back(tri);
        }
    }
    
    computeBounds();
    
    std::cout << "Loaded mesh with " << vertices.size() << " vertices and " 
              << triangles.size() << " triangles" << std::endl;
    
    return true;
}

void Mesh::computeBounds() {
    if (triangles.empty()) return;
    
    minBounds = glm::vec3(std::numeric_limits<float>::max());
    maxBounds = glm::vec3(std::numeric_limits<float>::lowest());
    
    for (const auto& tri : triangles) {
        for (const auto& vertex : {tri.v0, tri.v1, tri.v2}) {
            minBounds = glm::min(minBounds, vertex);
            maxBounds = glm::max(maxBounds, vertex);
        }
    }
}

glm::vec3 Mesh::computeTriangleNormal(const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2) {
    glm::vec3 edge1 = v1 - v0;
    glm::vec3 edge2 = v2 - v0;
    return glm::normalize(glm::cross(edge1, edge2));
}
