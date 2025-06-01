#include <iostream>
#include <string>
#include "renderer.h"
#include "simulation.h"

int main(int argc, char* argv[]) {
    int resolution = 64; // default
    
    if (argc > 1) {
        resolution = std::atoi(argv[1]);
        if (resolution <= 0) {
            std::cerr << "Invalid resolution: " << argv[1] << std::endl;
            return 1;
        }
    }
    
    std::cout << "SDF Collision Simulation" << std::endl;
    std::cout << "Resolution: " << resolution << "x" << resolution << "x" << resolution << std::endl;

    // Initialize renderer FIRST to setup OpenGL context
    Renderer renderer(800, 600);
    if (!renderer.initialize()) {
        std::cerr << "Failed to initialize renderer" << std::endl;
        return 1;
    }
    
    // Create collision object AFTER renderer initialization
    auto collisionObject = std::make_unique<CollisionObject>();
    if (!collisionObject->loadFromOBJ("../../data/bunny.obj", resolution)) {
        std::cerr << "Failed to load collision object" << std::endl;
        return 1;
    }    std::cout << "Collision object loaded. Calculating bounds from SDF..." << std::endl;
    std::cout << "Is collisionObject valid before move? " << (collisionObject && collisionObject->isValid() ? "Yes" : "No") << std::endl;
    
    // Calculate simulation bounds based on SDF size FIRST
    glm::vec3 objMin = collisionObject->getWorldMin();
    glm::vec3 objMax = collisionObject->getWorldMax();
    
    // Add padding around the object bounds for particle movement
    glm::vec3 padding = (objMax - objMin) * 0.5f;  // 50% padding on each side
    glm::vec3 boxMin = objMin - padding;
    glm::vec3 boxMax = objMax + padding;
    
    // Set custom mass for the collision object
    collisionObject->setMass(50.0f);  // Set as dynamic collision object
    
    // Move object to center of simulation bounds for better visibility
    glm::vec3 center = (boxMin + boxMax) * 0.5f;
    collisionObject->setPosition(center);
    
    // Set velocity to make the object move
    collisionObject->setVelocity(glm::vec3(1.0f, 0.5f, 0.0f));  // Move diagonally for more interesting motion
    
    std::cout << "Collision object mass: " << collisionObject->getMass() << std::endl;
    std::cout << "Collision object initial position: (" << center.x << ", " << center.y << ", " << center.z << ")" << std::endl;
    std::cout << "Collision object is static: " << (collisionObject->isStatic() ? "Yes" : "No") << std::endl;
    std::cout << "Collision object velocity: (" << collisionObject->getVelocity().x << ", " 
              << collisionObject->getVelocity().y << ", " << collisionObject->getVelocity().z << ")" << std::endl;    std::cout << "Mass-based collision response: " << (collisionObject->isStatic() ? "Simple reflection" : "Momentum conservation") << std::endl;
    
    std::cout << "Simulation bounds: (" << boxMin.x << "," << boxMin.y << "," << boxMin.z 
              << ") to (" << boxMax.x << "," << boxMax.y << "," << boxMax.z << ")" << std::endl;
    
    // Create simulation with calculated bounds
    Simulation simulation(boxMin, boxMax);
    
    // Add the collision object to the simulation
    simulation.addCollisionObject(std::move(collisionObject));
    
    // Calculate appropriate particle size based on mesh dimensions
    glm::vec3 objSize = objMax - objMin;
    float maxDimension = glm::max(glm::max(objSize.x, objSize.y), objSize.z);
    float particleSize = maxDimension * 0.01f;  // 1% of the largest object dimension
    float particleSpeed = maxDimension * 0.8f;   // 80% of the largest object dimension
    
    std::cout << "Object size: (" << objSize.x << ", " << objSize.y << ", " << objSize.z << ")" << std::endl;
    std::cout << "Calculated particle size: " << particleSize << std::endl;
    std::cout << "Calculated particle speed: " << particleSpeed << std::endl;
    std::cout << "Starting simulation..." << std::endl;
    
    // Initialize particles
    simulation.initialize(100, particleSpeed, particleSize);  // 100 particles

    // Timing for simulation updates
    auto lastTime = glfwGetTime();
    
    // Main rendering loop
    while (!renderer.shouldClose()) {
        // Calculate delta time
        auto currentTime = glfwGetTime();
        float deltaTime = static_cast<float>(currentTime - lastTime);
        lastTime = currentTime;
        
        // Handle ESC key
        if (glfwGetKey(renderer.getWindow(), GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(renderer.getWindow(), true);
        }
        
        // Update simulation (particles + collisions)
        simulation.update(deltaTime);
        
        renderer.beginFrame();
        
        // Draw wireframe box (get bounds from simulation)
        renderer.drawWireframeBox(simulation.getBoundsMin(), simulation.getBoundsMax());
        
        // Draw all collision objects
        const auto& collisionObjects = simulation.getCollisionObjects();
        for (const auto& obj : collisionObjects) {
            if (obj && obj->isValid()) {
                renderer.drawMesh(obj->getMesh(), obj->getPosition());
            }
        }
        
        // Draw particles
        renderer.drawParticles(simulation.getParticles());
        
        renderer.endFrame();
    }
    
    std::cout << "Simulation complete." << std::endl;
    
    return 0;
}
