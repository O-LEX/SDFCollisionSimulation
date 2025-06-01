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
    // Update collision object physics (position based on velocity)
    for (auto& obj : collisionObjects) {
        if (obj && obj->isValid()) {
            obj->updatePhysics(deltaTime);
        }
    }
    
    // Check collision object bounds and bounce if needed
    updateCollisionObjectBounds();
    
    // Update particle positions
    particleSystem.update(deltaTime);
    
    // Handle wall collisions
    handleWallCollisions();
    
    // Handle collisions with all collision objects
    handleMultipleCollisionObjectCollisions();
}

void Simulation::addCollisionObject(std::unique_ptr<CollisionObject> collisionObject) {
    std::cout << "Inside addCollisionObject. Is collisionObject valid? " << (collisionObject && collisionObject->isValid() ? "Yes" : "No") << std::endl;
    if (collisionObject && collisionObject->isValid()) {
        collisionObjects.push_back(std::move(collisionObject));
        std::cout << "Collision object added to vector. Vector size: " << collisionObjects.size() << std::endl;
    } else {
        std::cout << "Collision object NOT added to vector." << std::endl;
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
        for (auto& obj : collisionObjects) {
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
                    
                    // Calculate collision response considering masses
                    glm::vec3 newVelocity = calculateCollisionResponse(particle, *obj, normal);
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

glm::vec3 Simulation::calculateCollisionResponse(const Particle& particle, CollisionObject& object, const glm::vec3& normal) {
    // If collision object is static (infinite mass), use simple reflection
    if (object.isStatic()) {
        return reflectVelocity(particle.getVelocity(), normal);
    }
    
    // For dynamic collision objects, calculate velocities after collision using conservation of momentum
    float m1 = particle.getMass();
    float m2 = object.getMass();
    glm::vec3 v1 = particle.getVelocity();
    glm::vec3 v2 = object.getVelocity();  // Use actual collision object velocity
    
    // Calculate relative velocity
    glm::vec3 relativeVelocity = v1 - v2;
    float velocityAlongNormal = glm::dot(relativeVelocity, normal);
    
    // Do not resolve if velocities are separating
    if (velocityAlongNormal > 0) {
        return v1;
    }
    
    // Restitution coefficient (0 = perfectly inelastic, 1 = perfectly elastic)
    float restitution = 0.8f;
    
    // Calculate impulse scalar
    float j = -(1 + restitution) * velocityAlongNormal;
    j /= (particle.getInverseMass() + object.getInverseMass());
      // Apply impulse
    glm::vec3 impulse = j * normal;
    
    // Calculate new velocities for both particle and collision object
    glm::vec3 newParticleVelocity = v1 + particle.getInverseMass() * impulse;
    glm::vec3 newObjectVelocity = v2 - object.getInverseMass() * impulse;
    
    // Update collision object velocity
    object.setVelocity(newObjectVelocity);
    
    return newParticleVelocity;
}

void Simulation::updateCollisionObjectBounds() {
    for (auto& obj : collisionObjects) {
        if (!obj || obj->isStatic()) {
            continue;  // Skip static objects
        }
        
        glm::vec3 position = obj->getPosition();
        glm::vec3 velocity = obj->getVelocity();
        bool bounced = false;
        
        // Get object bounds in world space
        glm::vec3 objMin = obj->getWorldMin();
        glm::vec3 objMax = obj->getWorldMax();
        
        // Check X bounds
        if (objMin.x <= boundsMin.x) {
            velocity.x = glm::abs(velocity.x);  // Force positive velocity
            position.x = boundsMin.x + (position.x - objMin.x);  // Adjust position
            bounced = true;
        } else if (objMax.x >= boundsMax.x) {
            velocity.x = -glm::abs(velocity.x);  // Force negative velocity
            position.x = boundsMax.x - (objMax.x - position.x);  // Adjust position
            bounced = true;
        }
        
        // Check Y bounds
        if (objMin.y <= boundsMin.y) {
            velocity.y = glm::abs(velocity.y);  // Force positive velocity
            position.y = boundsMin.y + (position.y - objMin.y);  // Adjust position
            bounced = true;
        } else if (objMax.y >= boundsMax.y) {
            velocity.y = -glm::abs(velocity.y);  // Force negative velocity
            position.y = boundsMax.y - (objMax.y - position.y);  // Adjust position
            bounced = true;
        }
        
        // Check Z bounds
        if (objMin.z <= boundsMin.z) {
            velocity.z = glm::abs(velocity.z);  // Force positive velocity
            position.z = boundsMin.z + (position.z - objMin.z);  // Adjust position
            bounced = true;
        } else if (objMax.z >= boundsMax.z) {
            velocity.z = -glm::abs(velocity.z);  // Force negative velocity
            position.z = boundsMax.z - (objMax.z - position.z);  // Adjust position
            bounced = true;
        }
        
        // Update velocity and position if bounced
        if (bounced) {
            obj->setVelocity(velocity);
            obj->setPosition(position);
        }
    }
}
