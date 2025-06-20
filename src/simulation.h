#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <string>
#include <memory>
#include "particle.h"
#include "collision_object.h"

class Simulation {
public:
    Simulation(const glm::vec3& boxMin, const glm::vec3& boxMax);
    ~Simulation();
    
    void initialize(int numParticles = 100, float particleSpeed = 2.0f, float particleSize = 0.05f);
    void update(float deltaTime);
    
    // CollisionObject management
    void addCollisionObject(std::unique_ptr<CollisionObject> collisionObject);
    void clearCollisionObjects();
    
    const std::vector<Particle>& getParticles() const;
    const std::vector<std::unique_ptr<CollisionObject>>& getCollisionObjects() const { return collisionObjects; }
    size_t getCollisionObjectCount() const { return collisionObjects.size(); }
    const glm::vec3& getBoundsMin() const { return boundsMin; }
    const glm::vec3& getBoundsMax() const { return boundsMax; }
    
    void setParticleSize(float size);
    
    // Update collision object methods
    void updateCollisionObjectBounds();
    
    // Mesh-to-mesh collision methods
    void handleMeshToMeshCollisions();
    
private:
    ParticleSystem particleSystem;
    std::vector<std::unique_ptr<CollisionObject>> collisionObjects;
    glm::vec3 boundsMin, boundsMax;
    void handleWallCollisions();
    void handleMultipleCollisionObjectCollisions();
    void checkAndResolveObjectCollision(CollisionObject& obj1, CollisionObject& obj2);
    void resolveObjectCollision(CollisionObject& obj1, CollisionObject& obj2, 
                               const glm::vec3& pos1, const glm::vec3& pos2);
    glm::vec3 reflectVelocity(const glm::vec3& velocity, const glm::vec3& normal) const;
    glm::vec3 calculateCollisionResponse(const Particle& particle, CollisionObject& object, const glm::vec3& normal);
    bool checkWallCollision(const Particle& particle, glm::vec3& normal) const;
};
