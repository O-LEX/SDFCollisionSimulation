#pragma once

#include <glm/glm.hpp>
#include <vector>

class Particle {
public:
    Particle();
    Particle(const glm::vec3& position, const glm::vec3& velocity, float size = 0.05f, float mass = 1.0f);
    
    void update(float deltaTime);
    
    glm::vec3 getPosition() const { return position; }
    glm::vec3 getVelocity() const { return velocity; }
    float getSize() const { return size; }
    float getMass() const { return mass; }
    float getInverseMass() const { return inverseMass; }
    
    void setPosition(const glm::vec3& pos) { position = pos; }
    void setVelocity(const glm::vec3& vel) { velocity = vel; }
    void setSize(float s) { size = s; }
    void setMass(float m);
    
private:
    glm::vec3 position;
    glm::vec3 velocity;
    float size;
    float mass;
    float inverseMass;  // Cached for performance
};

class ParticleSystem {
public:
    ParticleSystem(int numParticles = 100);
    
    void initialize(const glm::vec3& boxMin, const glm::vec3& boxMax, float speed = 2.0f);
    void update(float deltaTime);
    
    const std::vector<Particle>& getParticles() const { return particles; }
    std::vector<Particle>& getParticles() { return particles; }
    void setParticleSize(float size);
    
private:
    std::vector<Particle> particles;
    int numParticles;
    
    glm::vec3 getRandomDirection() const;
    glm::vec3 getRandomPosition(const glm::vec3& boxMin, const glm::vec3& boxMax) const;
};
