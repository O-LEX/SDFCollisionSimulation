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
    
    // Handle mesh-to-mesh collisions
    handleMeshToMeshCollisions();
    
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
    float restitution = 1.0f;  // Perfect elastic collision
    
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

void Simulation::handleMeshToMeshCollisions() {
    // Check all pairs of collision objects for mesh-to-mesh collisions
    for (size_t i = 0; i < collisionObjects.size(); ++i) {
        for (size_t j = i + 1; j < collisionObjects.size(); ++j) {
            auto& obj1 = collisionObjects[i];
            auto& obj2 = collisionObjects[j];
            
            if (!obj1 || !obj2 || !obj1->isValid() || !obj2->isValid()) {
                continue;
            }
            
            // Skip if both objects are static
            if (obj1->isStatic() && obj2->isStatic()) {
                continue;
            }
            
            checkAndResolveObjectCollision(*obj1, *obj2);
        }
    }
}

void Simulation::checkAndResolveObjectCollision(CollisionObject& obj1, CollisionObject& obj2) {
    // Simple bounding box collision detection first
    glm::vec3 obj1Min = obj1.getWorldMin();
    glm::vec3 obj1Max = obj1.getWorldMax();
    glm::vec3 obj2Min = obj2.getWorldMin();
    glm::vec3 obj2Max = obj2.getWorldMax();
    
    // Check if bounding boxes overlap
    bool overlap = (obj1Min.x <= obj2Max.x && obj1Max.x >= obj2Min.x) &&
                  (obj1Min.y <= obj2Max.y && obj1Max.y >= obj2Min.y) &&
                  (obj1Min.z <= obj2Max.z && obj1Max.z >= obj2Min.z);
    
    if (!overlap) {
        return;
    }
    
    // More detailed collision detection using SDF
    glm::vec3 obj1Center = obj1.getPosition();
    glm::vec3 obj2Center = obj2.getPosition();
    
    // Sample obj2's SDF at obj1's center and vice versa
    float distance1 = obj2.getSignedDistance(obj1Center);
    float distance2 = obj1.getSignedDistance(obj2Center);
    
    // Also check multiple sample points for better accuracy
    float threshold = 0.02f; // Small positive threshold for surface contact
    
    bool collision = (distance1 < threshold) || (distance2 < threshold);
    
    if (collision) {
        std::cout << "Collision detected! Distances: " << distance1 << ", " << distance2 << std::endl;
        resolveObjectCollision(obj1, obj2, obj1Center, obj2Center);
    }
}

void Simulation::resolveObjectCollision(CollisionObject& obj1, CollisionObject& obj2, 
                                       const glm::vec3& pos1, const glm::vec3& pos2) {
    std::cout << "Starting collision resolution..." << std::endl;
    
    // Calculate collision normal (from obj1 to obj2)
    glm::vec3 normal = pos2 - pos1;
    float distance = glm::length(normal);
    
    if (distance < 0.001f) {
        // Objects are at the same position, use default separation
        normal = glm::vec3(1.0f, 0.0f, 0.0f);
        distance = 0.001f;
    } else {
        normal = glm::normalize(normal);
    }
    
    // Calculate actual penetration depth using SDF
    float penetrationDepth = 0.0f;
    float dist1 = obj2.getSignedDistance(pos1);
    float dist2 = obj1.getSignedDistance(pos2);
    
    // If distance is negative, objects are penetrating
    if (dist1 < 0.0f) penetrationDepth = glm::max(penetrationDepth, -dist1);
    if (dist2 < 0.0f) penetrationDepth = glm::max(penetrationDepth, -dist2);
    
    // If no penetration detected but we're here, use small separation
    if (penetrationDepth == 0.0f) {
        penetrationDepth = 0.05f; // Small default separation
    }
    
    // Use separation based on penetration
    float separation = glm::max(0.02f, penetrationDepth * 1.2f);  // 20% buffer
    glm::vec3 separationVector = normal * separation * 0.5f;
    
    // Store original positions for smoother adjustment
    glm::vec3 originalPos1 = pos1;
    glm::vec3 originalPos2 = pos2;
    
    if (!obj1.isStatic()) {
        obj1.setPosition(originalPos1 - separationVector);
    }
    if (!obj2.isStatic()) {
        obj2.setPosition(originalPos2 + separationVector);
    }
    
    // Calculate collision response
    glm::vec3 v1 = obj1.getVelocity();
    glm::vec3 v2 = obj2.getVelocity();
    
    float m1 = obj1.getMass();
    float m2 = obj2.getMass();
    
    std::cout << "Collision response - Masses: " << m1 << ", " << m2 
              << ", Penetration: " << penetrationDepth << ", Distance: " << distance << std::endl;
    std::cout << "Velocities before: v1=(" << v1.x << "," << v1.y << "," << v1.z 
              << ") v2=(" << v2.x << "," << v2.y << "," << v2.z << ")" << std::endl;
      // Handle static objects
    if (obj1.isStatic()) {
        // obj1 is static, only obj2 bounces
        glm::vec3 newV2 = reflectVelocity(v2, -normal);
        obj2.setVelocity(newV2);  // No damping - perfect reflection
        std::cout << "Static collision resolved (obj1 static)" << std::endl;
        return;
    }
    if (obj2.isStatic()) {
        // obj2 is static, only obj1 bounces
        glm::vec3 newV1 = reflectVelocity(v1, normal);
        obj1.setVelocity(newV1);  // No damping - perfect reflection
        std::cout << "Static collision resolved (obj2 static)" << std::endl;
        return;
    }
      // Both objects are dynamic - calculate collision response
    glm::vec3 relativeVelocity = v1 - v2;
    float velocityAlongNormal = glm::dot(relativeVelocity, normal);
    
    std::cout << "Relative velocity along normal: " << velocityAlongNormal << std::endl;
      // Always apply collision response for interpenetrating objects
    
    // Restitution coefficient (bounce) - perfect elastic collision
    float restitution = 1.0f;
    
    // Calculate impulse scalar
    float j = -(1 + restitution) * velocityAlongNormal;
    j /= (obj1.getInverseMass() + obj2.getInverseMass());
    
    // Apply impulse
    glm::vec3 impulse = j * normal;
    
    glm::vec3 newV1 = v1 + obj1.getInverseMass() * impulse;
    glm::vec3 newV2 = v2 - obj2.getInverseMass() * impulse;
    
    obj1.setVelocity(newV1);
    obj2.setVelocity(newV2);
    
    std::cout << "Dynamic mesh collision resolved! Final velocities: (" 
              << newV1.x << "," << newV1.y << "," << newV1.z << ") and ("
              << newV2.x << "," << newV2.y << "," << newV2.z << ")" << std::endl;
}
