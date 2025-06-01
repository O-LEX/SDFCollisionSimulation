#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <memory>
#include <string>
#include "mesh.h"
#include "sdf.h"

class CollisionObject {
public:
    CollisionObject();
    ~CollisionObject() = default;
    
    // Initialization
    bool loadFromOBJ(const std::string& filename, int sdfResolution = 64);
    
    // Transform operations
    void setPosition(const glm::vec3& position);
    void setRotation(const glm::quat& rotation);
    void setScale(const glm::vec3& scale);
    
    const glm::vec3& getPosition() const { return position; }
    const glm::quat& getRotation() const { return rotation; }
    const glm::vec3& getScale() const { return scale; }
    
    // Access to mesh and SDF
    const Mesh& getMesh() const { return mesh; }
    const SDF& getSDF() const { return sdf; }
    
    // Collision detection
    float getSignedDistance(const glm::vec3& worldPosition) const;
    glm::vec3 getNormal(const glm::vec3& worldPosition) const;
    
    // Transform matrices
    glm::mat4 getTransformMatrix() const;
    glm::mat4 getInverseTransformMatrix() const;
    
    // Bounds in world space
    glm::vec3 getWorldMin() const;
    glm::vec3 getWorldMax() const;
    
    // Validation
    bool isValid() const { return meshLoaded && sdfGenerated; }
    
private:
    Mesh mesh;
    SDF sdf;
    
    // Transform properties
    glm::vec3 position;
    glm::quat rotation;
    glm::vec3 scale;
    
    // Cached transform matrices
    mutable glm::mat4 transformMatrix;
    mutable glm::mat4 inverseTransformMatrix;
    mutable bool transformDirty;
    
    // State flags
    bool meshLoaded;
    bool sdfGenerated;
    
    // Helper methods
    void updateTransformCache() const;
    glm::vec3 worldToLocal(const glm::vec3& worldPos) const;
    glm::vec3 localToWorld(const glm::vec3& localPos) const;
    glm::vec3 transformNormal(const glm::vec3& localNormal) const;
};
