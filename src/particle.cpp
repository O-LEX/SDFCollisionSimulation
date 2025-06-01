#include "particle.h"
#include <random>
#include <cmath>

Particle::Particle() : position(0.0f), velocity(0.0f), size(0.05f), mass(1.0f), inverseMass(1.0f) {}

Particle::Particle(const glm::vec3& position, const glm::vec3& velocity, float size, float mass)
    : position(position), velocity(velocity), size(size) {
    setMass(mass);
}

void Particle::setMass(float m) {
    this->mass = m;
    if (m > 0.0f) {
        inverseMass = 1.0f / m;
    } else {
        inverseMass = 0.0f;  // Infinite mass (static)
    }
}

void Particle::update(float deltaTime) {
    position += velocity * deltaTime;
}

ParticleSystem::ParticleSystem(int numParticles) : numParticles(numParticles) {
    particles.reserve(numParticles);
}

void ParticleSystem::initialize(const glm::vec3& boxMin, const glm::vec3& boxMax, float speed) {
    particles.clear();
    particles.reserve(numParticles);
      for (int i = 0; i < numParticles; ++i) {
        glm::vec3 pos = getRandomPosition(boxMin, boxMax);
        glm::vec3 vel = getRandomDirection() * speed;
        float particleMass = 1.0f;  // Default particle mass
        particles.emplace_back(pos, vel, 0.05f, particleMass);
    }
}

void ParticleSystem::update(float deltaTime) {
    for (auto& particle : particles) {
        particle.update(deltaTime);
    }
}

void ParticleSystem::setParticleSize(float size) {
    for (auto& particle : particles) {
        particle.setSize(size);
    }
}

glm::vec3 ParticleSystem::getRandomDirection() const {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_real_distribution<float> dis(-1.0f, 1.0f);
    
    glm::vec3 direction;
    do {
        direction = glm::vec3(dis(gen), dis(gen), dis(gen));
    } while (glm::length(direction) > 1.0f || glm::length(direction) < 0.1f);
    
    return glm::normalize(direction);
}

glm::vec3 ParticleSystem::getRandomPosition(const glm::vec3& boxMin, const glm::vec3& boxMax) const {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_real_distribution<float> disX(boxMin.x, boxMax.x);
    static std::uniform_real_distribution<float> disY(boxMin.y, boxMax.y);
    static std::uniform_real_distribution<float> disZ(boxMin.z, boxMax.z);
    
    return glm::vec3(disX(gen), disY(gen), disZ(gen));
}
