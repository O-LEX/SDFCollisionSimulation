#include "simulation.h"
#include <algorithm>
#include <iostream>

Simulation::Simulation(const glm::vec3& boxMin, const glm::vec3& boxMax) 
    : boundsMin(boxMin), boundsMax(boxMax), particleSystem(100) {
}

Simulation::~Simulation() {
    // std::unique_ptr automatically handles cleanup
}

void Simulation::initialize(int numParticles, float particleSpeed, float particleSize) {
    particleSystem = ParticleSystem(numParticles);
    particleSystem.initialize(boundsMin, boundsMax, particleSpeed);
    particleSystem.setParticleSize(particleSize);
}

void Simulation::update(float deltaTime) {
    // Update particle positions
    particleSystem.update(deltaTime);
    
    // Handle wall collisions
    handleWallCollisions();
    
    // Handle collisions with all collision objects
    handleMultipleCollisionObjectCollisions();
}

void Simulation::addCollisionObject(std::unique_ptr<CollisionObject> collisionObject) {
    if (collisionObject && collisionObject->isValid()) {
        collisionObjects.push_back(std::move(collisionObject));
    }
}



void Simulation::clearCollisionObjects() {
    collisionObjects.clear();
}

const std::vector<Particle>& Simulation::getParticles() const {
    return particleSystem.getParticles();
}

void Simulation::setParticleSize(float size) {
    particleSystem.setParticleSize(size);
}

void Simulation::handleWallCollisions() {
    // Get non-const reference to particles for modification
    std::vector<Particle>& particles = particleSystem.getParticles();
    
    for (auto& particle : particles) {
        glm::vec3 normal;
        if (checkWallCollision(particle, normal)) {
            // Reflect velocity
            glm::vec3 newVelocity = reflectVelocity(particle.getVelocity(), normal);
            particle.setVelocity(newVelocity);
            
            // Correct position to prevent penetration
            glm::vec3 pos = particle.getPosition();
            float radius = particle.getSize();
            
            // Push particle back inside bounds
            if (pos.x - radius < boundsMin.x) pos.x = boundsMin.x + radius;
            if (pos.x + radius > boundsMax.x) pos.x = boundsMax.x - radius;
            if (pos.y - radius < boundsMin.y) pos.y = boundsMin.y + radius;
            if (pos.y + radius > boundsMax.y) pos.y = boundsMax.y - radius;
            if (pos.z - radius < boundsMin.z) pos.z = boundsMin.z + radius;
            if (pos.z + radius > boundsMax.z) pos.z = boundsMax.z - radius;
            
            particle.setPosition(pos);
        }
    }
}

glm::vec3 Simulation::reflectVelocity(const glm::vec3& velocity, const glm::vec3& normal) const {
    // Reflection formula: v' = v - 2 * dot(v, n) * n
    // where v is velocity, n is normal, v' is reflected velocity
    return velocity - 2.0f * glm::dot(velocity, normal) * normal;
}

bool Simulation::checkWallCollision(const Particle& particle, glm::vec3& normal) const {
    glm::vec3 pos = particle.getPosition();
    float radius = particle.getSize();
    
    bool collision = false;
    normal = glm::vec3(0.0f);
    
    // Check X walls
    if (pos.x - radius <= boundsMin.x) {
        normal.x = 1.0f;  // Normal pointing inward (positive X)
        collision = true;
    } else if (pos.x + radius >= boundsMax.x) {
        normal.x = -1.0f; // Normal pointing inward (negative X)
        collision = true;
    }
    
    // Check Y walls
    if (pos.y - radius <= boundsMin.y) {
        normal.y = 1.0f;  // Normal pointing inward (positive Y)
        collision = true;
    } else if (pos.y + radius >= boundsMax.y) {
        normal.y = -1.0f; // Normal pointing inward (negative Y)
        collision = true;
    }
    
    // Check Z walls
    if (pos.z - radius <= boundsMin.z) {
        normal.z = 1.0f;  // Normal pointing inward (positive Z)
        collision = true;
    } else if (pos.z + radius >= boundsMax.z) {
        normal.z = -1.0f; // Normal pointing inward (negative Z)
        collision = true;
    }
    
    // Normalize the normal vector if multiple walls are hit (corner case)
    if (collision && glm::length(normal) > 1.0f) {
        normal = glm::normalize(normal);
    }
    
    return collision;
}

void Simulation::handleMultipleCollisionObjectCollisions() {
    if (collisionObjects.empty()) return;
    
    std::vector<Particle>& particles = particleSystem.getParticles();
    
    for (auto& particle : particles) {
        glm::vec3 pos = particle.getPosition();
        float radius = particle.getSize();
        
        // Check collision with each collision object
        for (const auto& obj : collisionObjects) {
            if (!obj || !obj->isValid()) continue;
            
            // Sample SDF at particle position
            float distance = obj->getSignedDistance(pos);
            
            // Check for collision (particle inside or very close to mesh surface)
            if (distance < radius) {
                // Get surface normal using SDF gradient
                glm::vec3 normal = obj->getNormal(pos);
                
                // Normalize if not zero
                if (glm::length(normal) > 0.001f) {
                    normal = glm::normalize(normal);
                    
                    // Reflect velocity
                    glm::vec3 newVelocity = reflectVelocity(particle.getVelocity(), normal);
                    particle.setVelocity(newVelocity);
                    
                    // Push particle outside mesh surface
                    glm::vec3 correctedPos = pos + normal * (radius - distance + 0.001f);
                    particle.setPosition(correctedPos);
                    
                    // Break after first collision to avoid double corrections
                    break;
                }
            }
        }
    }
}
