#include <iostream>
#include <string>
#include "mesh.h"
#include "sdf.h"
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
    
    // Load mesh
    Mesh mesh;
    if (!mesh.loadOBJ("data/stanford-bunny.obj")) {
        std::cerr << "Failed to load mesh" << std::endl;
        return 1;
    }
    
    // Generate SDF
    SDF sdf(resolution);
    sdf.generateFromMesh(mesh);
    
    std::cout << "SDF generation complete. Starting simulation..." << std::endl;
    
    // Initialize renderer
    Renderer renderer(800, 600);
    if (!renderer.initialize()) {
        std::cerr << "Failed to initialize renderer" << std::endl;
        return 1;
    }
    
    // Initialize simulation with box bounds based on SDF
    glm::vec3 sdfMin = sdf.getMin();
    glm::vec3 sdfMax = sdf.getMax();
    
    // Add padding around the SDF bounds for particle movement
    glm::vec3 padding = (sdfMax - sdfMin) * 0.5f;  // 50% padding on each side
    glm::vec3 boxMin = sdfMin - padding;
    glm::vec3 boxMax = sdfMax + padding;
    
    std::cout << "Simulation bounds: (" << boxMin.x << "," << boxMin.y << "," << boxMin.z 
              << ") to (" << boxMax.x << "," << boxMax.y << "," << boxMax.z << ")" << std::endl;
    
    Simulation simulation(boxMin, boxMax);
    
    // Set mesh and SDF in simulation
    simulation.setMesh(mesh);
    simulation.setSDF(sdf);
    
    // Calculate appropriate particle size based on mesh dimensions
    glm::vec3 meshSize = sdfMax - sdfMin;
    float maxDimension = glm::max(glm::max(meshSize.x, meshSize.y), meshSize.z);
    float particleSize = maxDimension * 0.01f;  // 1.5% of the largest mesh dimension
    float particleSpeed = maxDimension * 0.8f;   // 80% of the largest mesh dimension for realistic movement
    
    std::cout << "Mesh size: (" << meshSize.x << ", " << meshSize.y << ", " << meshSize.z << ")" << std::endl;
    std::cout << "Calculated particle size: " << particleSize << std::endl;
    std::cout << "Calculated particle speed: " << particleSpeed << std::endl;
    
    // Initialize particles
    simulation.initialize(100, particleSpeed, particleSize);  // 100 particles, calculated speed & size
    simulation.setMeshPosition(glm::vec3(0.0f, 0.0f, 0.0f));  // Mesh at origin
    
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
        
        // Draw mesh
        renderer.drawMesh(simulation.getMesh(), simulation.getMeshPosition());
        
        // Draw particles
        renderer.drawParticles(simulation.getParticles());
        
        renderer.endFrame();
    }
    
    std::cout << "Simulation complete." << std::endl;
    
    return 0;
}
