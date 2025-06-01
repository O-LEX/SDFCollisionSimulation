#include <iostream>
#include <string>
#include <memory>
#include "renderer.h"
#include "collision_object.h"
#include "simulation.h"

int main(int argc, char* argv[]) {
    int resolution = 32; // Lower resolution for faster computation
    
    if (argc > 1) {
        resolution = std::atoi(argv[1]);
        if (resolution <= 0) {
            std::cerr << "Invalid resolution: " << argv[1] << std::endl;
            return 1;
        }
    }
    
    std::cout << "Mesh Collision Simulation" << std::endl;
    std::cout << "Resolution: " << resolution << "x" << resolution << "x" << resolution << std::endl;

    // Initialize renderer
    Renderer renderer(800, 600);
    if (!renderer.initialize()) {
        std::cerr << "Failed to initialize renderer" << std::endl;
        return 1;
    }
    
    // First, create collision objects to analyze their sizes
    // Create first collision object
    auto obj1 = std::make_unique<CollisionObject>();
    if (!obj1->loadFromOBJ("../../data/bunny.obj", resolution)) {
        std::cerr << "Failed to load first collision object" << std::endl;
        return 1;
    }
    
    // Create second collision object  
    auto obj2 = std::make_unique<CollisionObject>();
    if (!obj2->loadFromOBJ("../../data/bunny.obj", resolution)) {
        std::cerr << "Failed to load second collision object" << std::endl;
        return 1;
    }
    
    // Create third collision object (static)
    auto obj3 = std::make_unique<CollisionObject>();
    if (!obj3->loadFromOBJ("../../data/bunny.obj", resolution)) {
        std::cerr << "Failed to load third collision object" << std::endl;
        return 1;
    }
      // Calculate object sizes and determine simulation bounds
    glm::vec3 objSize = obj1->getMesh().getMax() - obj1->getMesh().getMin();
    float maxDimension = glm::max(glm::max(objSize.x, objSize.y), objSize.z);
    
    std::cout << "Object size: (" << objSize.x << ", " << objSize.y << ", " << objSize.z << ")" << std::endl;
    std::cout << "Max dimension: " << maxDimension << std::endl;
      // Set simulation bounds based on object size
    // Make the simulation area smaller - only 5x larger than the largest object
    float simSize = maxDimension * 2.5f;
    glm::vec3 boundsMin(-simSize, -simSize * 0.6f, -simSize);
    glm::vec3 boundsMax(simSize, simSize * 0.6f, simSize);
    
    std::cout << "Simulation bounds: Min(" << boundsMin.x << ", " << boundsMin.y << ", " << boundsMin.z << ")" << std::endl;
    std::cout << "                   Max(" << boundsMax.x << ", " << boundsMax.y << ", " << boundsMax.z << ")" << std::endl;
    
    // Initialize simulation with calculated bounds
    Simulation simulation(boundsMin, boundsMax);
    // We don't need particles for this demo, so skip particle initialization    // Configure objects - simpler setup with only 2 dynamic objects for testing
    float spacing = maxDimension * 1.5f; // Closer spacing for guaranteed collision
    
    obj1->setMass(10.0f);
    obj1->setPosition(glm::vec3(-spacing, 0.0f, 0.0f));
    obj1->setVelocity(glm::vec3(maxDimension * 0.8f, 0.0f, 0.0f)); // Moderate velocity
    
    obj2->setMass(15.0f);
    obj2->setPosition(glm::vec3(spacing, 0.0f, 0.0f)); // Same Y level for direct collision
    obj2->setVelocity(glm::vec3(-maxDimension * 0.6f, 0.0f, 0.0f));
    obj2->setScale(glm::vec3(2.0f, 2.0f, 2.0f));
    
    obj3->setMass(2.0f);
    obj3->setPosition(glm::vec3(0.0f, maxDimension * 0.5f, 0.0f));
    obj3->setScale(glm::vec3(2.0f, 0.5f, 2.0f));  // Flat platform
    
    // Add objects to simulation
    simulation.addCollisionObject(std::move(obj1));
    simulation.addCollisionObject(std::move(obj2));
    simulation.addCollisionObject(std::move(obj3));
    
    // Access objects through simulation after move
    const auto& objects = simulation.getCollisionObjects();
    std::cout << "Starting mesh collision simulation..." << std::endl;
    std::cout << "Objects: 2 dynamic (masses " << objects[0]->getMass() << ", " << objects[1]->getMass() << "), 1 static platform" << std::endl;
    std::cout << "Spacing: " << spacing << ", Max dimension: " << maxDimension << std::endl;
    
    // Timing for simulation updates
    auto lastTime = glfwGetTime();
    
    // Main rendering loop
    while (!renderer.shouldClose()) {        // Calculate delta time
        auto currentTime = glfwGetTime();
        float deltaTime = static_cast<float>(currentTime - lastTime);
        lastTime = currentTime;
        
        // Limit delta time to prevent large jumps and ensure smooth physics
        deltaTime = glm::min(deltaTime, 0.008f);  // Cap at ~120 FPS for smoother collisions
        
        // Handle ESC key
        if (glfwGetKey(renderer.getWindow(), GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(renderer.getWindow(), true);
        }
        
        // Update simulation with smaller time steps for stability
        simulation.update(deltaTime);
        
        renderer.beginFrame();
        
        // Draw wireframe box (simulation bounds)
        renderer.drawWireframeBox(simulation.getBoundsMin(), simulation.getBoundsMax());
          // Draw all collision objects
        const auto& collisionObjects = simulation.getCollisionObjects();
        for (const auto& obj : collisionObjects) {
            if (obj && obj->isValid()) {
                renderer.drawMesh(obj->getMesh(), obj->getTransformMatrix());
            }
        }
        
        renderer.endFrame();
    }
    
    std::cout << "Simulation complete." << std::endl;
    
    return 0;
}
