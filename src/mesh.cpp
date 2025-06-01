#include "mesh.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <limits>
#include <GL/glew.h> // Added for OpenGL functions
#include <map> // Added for std::map

// Custom comparator for glm::vec3 keys in std::map
struct Vec3Compare {
    bool operator()(const glm::vec3& a, const glm::vec3& b) const {
        if (a.x != b.x) return a.x < b.x;
        if (a.y != b.y) return a.y < b.y;
        return a.z < b.z;
    }
};

Mesh::Mesh() : VAO(0), VBO(0), EBO(0), meshSetupDone(false) {
}

Mesh::~Mesh() {
    if (meshSetupDone) {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
        glDeleteBuffers(1, &EBO);
    }
}

bool Mesh::loadOBJ(const std::string& filename) {
    std::cout << "Mesh::loadOBJ - Attempting to load: " << filename << std::endl; // Added log
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Mesh::loadOBJ - Failed to open OBJ file: " << filename << std::endl;
        return false;
    }
    std::cout << "Mesh::loadOBJ - File opened successfully." << std::endl; // Added log
    
    std::vector<glm::vec3> loaded_vertices_local; // Renamed from loaded_vertices
    std::vector<std::vector<int>> faces;
    
    std::string line;
    int line_number = 0;
    while (std::getline(file, line)) {
        line_number++;
        std::istringstream iss(line);
        std::string prefix;
        iss >> prefix;
        
        if (prefix == "v") {
            float x, y, z;
            if (!(iss >> x >> y >> z)) {
                std::cerr << "Mesh::loadOBJ - Error parsing vertex data at line " << line_number << ": " << line << std::endl;
                continue;
            }
            loaded_vertices_local.push_back(glm::vec3(x, y, z));
            // std::cout << "Mesh::loadOBJ - Loaded vertex: " << x << ", " << y << ", " << z << std::endl; // Optional: verbose log
        }
        else if (prefix == "f") {
            // std::cout << "Mesh::loadOBJ - Processing face line " << line_number << ": " << line << std::endl; // Added log
            std::vector<int> face_indices;
            std::string vertex_str;
            while (iss >> vertex_str) {
                // std::cout << "Mesh::loadOBJ - Parsing face component: " << vertex_str << std::endl; // Added log
                size_t slash_pos = vertex_str.find('/');
                int vertex_index = -1;
                try {
                    vertex_index = std::stoi(vertex_str.substr(0, slash_pos)) - 1; // OBJ is 1-indexed
                } catch (const std::invalid_argument& ia) {
                    std::cerr << "Mesh::loadOBJ - Invalid argument for std::stoi at line " << line_number << " for component '" << vertex_str.substr(0, slash_pos) << "': " << ia.what() << std::endl;
                    return false; // Critical error
                } catch (const std::out_of_range& oor) {
                    std::cerr << "Mesh::loadOBJ - Out of range for std::stoi at line " << line_number << " for component '" << vertex_str.substr(0, slash_pos) << "': " << oor.what() << std::endl;
                    return false; // Critical error
                }
                
                if (vertex_index < 0 /*|| static_cast<size_t>(vertex_index) >= loaded_vertices_local.size() // This check will be done later when constructing triangles */) {
                    // We can't check upper bound yet as all 'v' lines might not have been read if 'f' lines are interleaved.
                    // However, OBJ standard usually puts all vertices first. A very large index might indicate a problem.
                    // For now, we only check for negative index from stoi.
                     std::cerr << "Mesh::loadOBJ - Invalid vertex index " << vertex_index + 1 << " (parsed " << vertex_index << ") at line " << line_number << std::endl;
                    // return false; // Potentially too strict if file is malformed but still processable.
                                     // We will validate indices when creating triangles.
                }
                face_indices.push_back(vertex_index);
            }
            if (face_indices.size() >= 3) {
                faces.push_back(face_indices);
            } else if (!face_indices.empty()) {
                 std::cerr << "Mesh::loadOBJ - Face with < 3 vertices at line " << line_number << ": " << line << std::endl;
            }
        }
    }
    std::cout << "Mesh::loadOBJ - Initial parsing complete. Loaded " << loaded_vertices_local.size() << " vertices and " << faces.size() << " faces." << std::endl; // Added log
    
    // Convert faces to triangles (for SDF) and prepare rendering data
    triangles.clear();
    vertices_for_rendering.clear();
    indices_for_rendering.clear();
    
    std::vector<glm::vec3> unique_vertices_for_rendering_temp; // Renamed to avoid conflict with member
    std::map<glm::vec3, unsigned int, Vec3Compare> vertex_to_index_map;

    auto get_or_add_vertex = [&](const glm::vec3& v) -> unsigned int { // Explicit return type
        if (vertex_to_index_map.find(v) == vertex_to_index_map.end()) {
            vertex_to_index_map[v] = static_cast<unsigned int>(unique_vertices_for_rendering_temp.size());
            unique_vertices_for_rendering_temp.push_back(v);
        }
        return vertex_to_index_map[v];
    };

    std::cout << "Mesh::loadOBJ - Starting triangle conversion and rendering data preparation." << std::endl; // Added log
    for (const auto& face : faces) {
        // Triangulate if face has more than 3 vertices
        for (size_t i = 1; i < face.size() - 1; ++i) {
            int idx0 = face[0];
            int idx1 = face[i];
            int idx2 = face[i + 1];

            // Bounds checking for vertex indices
            if (idx0 < 0 || static_cast<size_t>(idx0) >= loaded_vertices_local.size() ||
                idx1 < 0 || static_cast<size_t>(idx1) >= loaded_vertices_local.size() ||
                idx2 < 0 || static_cast<size_t>(idx2) >= loaded_vertices_local.size()) {
                std::cerr << "Mesh::loadOBJ - Vertex index out of bounds when creating triangle. Face indices: "
                          << idx0 << ", " << idx1 << ", " << idx2
                          << ". Total loaded vertices: " << loaded_vertices_local.size() << std::endl;
                return false; // Critical error
            }

            Triangle tri;
            tri.v0 = loaded_vertices_local[idx0];
            tri.v1 = loaded_vertices_local[idx1];
            tri.v2 = loaded_vertices_local[idx2];
            tri.normal = computeTriangleNormal(tri.v0, tri.v1, tri.v2);
            triangles.push_back(tri); // For SDF

            // For rendering with indices
            indices_for_rendering.push_back(get_or_add_vertex(tri.v0));
            indices_for_rendering.push_back(get_or_add_vertex(tri.v1));
            indices_for_rendering.push_back(get_or_add_vertex(tri.v2));
        }
    }
    this->vertices_for_rendering = unique_vertices_for_rendering_temp; // Assign to member variable
    std::cout << "Mesh::loadOBJ - Triangle conversion complete." << std::endl; // Added log

    computeBounds();
    std::cout << "Mesh::loadOBJ - Bounds computed. Min: (" << minBounds.x << "," << minBounds.y << "," << minBounds.z 
              << "), Max: (" << maxBounds.x << "," << maxBounds.y << "," << maxBounds.z << ")" << std::endl; // Added log
    
    setupMesh(); // Setup VAO/VBO/EBO after loading data
    std::cout << "Mesh::loadOBJ - Mesh setup for rendering complete." << std::endl; // Added log
    
    std::cout << "Loaded mesh with " << loaded_vertices_local.size() << " raw input vertices, " 
              << this->vertices_for_rendering.size() << " unique rendering vertices, and " 
              << triangles.size() << " triangles (for SDF), "
              << indices_for_rendering.size() / 3 << " rendering triangles." << std::endl;
    
    return true;
}

void Mesh::setupMesh() {
    std::cout << "Mesh::setupMesh - Setting up VAO/VBO/EBO." << std::endl; // Added log
    if (vertices_for_rendering.empty() || indices_for_rendering.empty()) {
        std::cerr << "Cannot setup mesh: No vertex or index data." << std::endl;
        return;
    }

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices_for_rendering.size() * sizeof(glm::vec3), vertices_for_rendering.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices_for_rendering.size() * sizeof(unsigned int), indices_for_rendering.data(), GL_STATIC_DRAW);

    // Vertex positions
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);

    glBindVertexArray(0); // Unbind VAO
    meshSetupDone = true;
    std::cout << "Mesh::setupMesh - VAO/VBO/EBO setup done." << std::endl; // Added log
}

void Mesh::draw() const {
    if (!meshSetupDone) {
        std::cerr << "Mesh not setup for drawing!" << std::endl;
        return;
    }
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices_for_rendering.size()), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
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
