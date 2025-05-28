#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <string>
#include "particle.h"
#include "mesh.h"
#include "sdf.h"

class Simulation {
public:
    Simulation(const glm::vec3& boxMin, const glm::vec3& boxMax);
    ~Simulation();
    
    void initialize(int numParticles = 100, float particleSpeed = 2.0f, float particleSize = 0.05f);
    void update(float deltaTime);
    
    void setMesh(const Mesh& meshRef);
    void setSDF(const SDF& sdfRef);
    
    const std::vector<Particle>& getParticles() const;
    const Mesh& getMesh() const { return *mesh; }
    const glm::vec3& getMeshPosition() const { return meshPosition; }
    const glm::vec3& getBoundsMin() const { return boundsMin; }
    const glm::vec3& getBoundsMax() const { return boundsMax; }
    
    void setParticleSize(float size);
    void setMeshPosition(const glm::vec3& position);
    
private:
    ParticleSystem particleSystem;
    const Mesh* mesh;        // Pointer to external mesh
    const SDF* sdf;          // Pointer to external SDF
    glm::vec3 meshPosition;
    glm::vec3 boundsMin, boundsMax;
    bool meshSet;
    bool sdfSet;
    
    void handleWallCollisions();
    void handleMeshCollisions();
    glm::vec3 reflectVelocity(const glm::vec3& velocity, const glm::vec3& normal) const;
    bool checkWallCollision(const Particle& particle, glm::vec3& normal) const;
};
