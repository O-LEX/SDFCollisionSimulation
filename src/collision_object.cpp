#include "collision_object.h"
#include <glm/gtc/matrix_inverse.hpp>
#include <limits>

CollisionObject::CollisionObject() 
    : sdf(64), position(0.0f), rotation(1.0f, 0.0f, 0.0f, 0.0f), scale(1.0f), velocity(0.0f),
      mass(0.0f), inverseMass(0.0f),  // Default to static object (infinite mass)
      transformMatrix(1.0f), inverseTransformMatrix(1.0f), transformDirty(true),
      meshLoaded(false), sdfGenerated(false) {
}

bool CollisionObject::loadFromOBJ(const std::string& filename, int sdfResolution) {
    // Load mesh
    if (!mesh.loadOBJ(filename)) {
        meshLoaded = false;
        sdfGenerated = false;
        return false;
    }
    meshLoaded = true;
      // Generate SDF with specified resolution
    sdf = SDF(sdfResolution);
    sdf.generateFromMesh(mesh);
    sdfGenerated = true;
    
    // Calculate default mass based on mesh volume approximation
    glm::vec3 meshSize = mesh.getMax() - mesh.getMin();
    float volume = meshSize.x * meshSize.y * meshSize.z;
    float defaultDensity = 1.0f;  // Assume unit density
    setMass(volume * defaultDensity);
    
    // Mark transform as dirty to ensure matrices are recalculated
    transformDirty = true;
    
    return true;
}

void CollisionObject::setPosition(const glm::vec3& position) {
    this->position = position;
    transformDirty = true;
}

void CollisionObject::setRotation(const glm::quat& rotation) {
    this->rotation = rotation;
    transformDirty = true;
}

void CollisionObject::setScale(const glm::vec3& scale) {
    this->scale = scale;
    transformDirty = true;
}

void CollisionObject::setVelocity(const glm::vec3& velocity) {
    this->velocity = velocity;
}

void CollisionObject::updatePhysics(float deltaTime) {
    if (!isStatic() && deltaTime > 0.0f) {
        // Update position based on velocity
        setPosition(position + velocity * deltaTime);
    }
}

void CollisionObject::setMass(float mass) {
    this->mass = mass;
    if (mass > 0.0f) {
        inverseMass = 1.0f / mass;
    } else {
        inverseMass = 0.0f;  // Static object (infinite mass)
    }
}

float CollisionObject::getSignedDistance(const glm::vec3& worldPosition) const {
    if (!isValid()) {
        return std::numeric_limits<float>::max();
    }
    
    // Transform world position to local space
    glm::vec3 localPos = worldToLocal(worldPosition);
    
    // Sample SDF in local space
    float localDistance = sdf.sample(localPos);
    
    // Scale the distance by the minimum scale factor
    // This is an approximation - for non-uniform scaling, 
    // the distance field becomes more complex
    float minScale = std::min({scale.x, scale.y, scale.z});
    return localDistance * minScale;
}

glm::vec3 CollisionObject::getNormal(const glm::vec3& worldPosition) const {
    if (!isValid()) {
        return glm::vec3(0.0f, 1.0f, 0.0f); // Default up vector
    }
    
    // Transform world position to local space
    glm::vec3 localPos = worldToLocal(worldPosition);
    
    // Get gradient (normal) in local space
    glm::vec3 localNormal = sdf.gradient(localPos);
    
    // Transform normal back to world space
    return transformNormal(localNormal);
}

glm::mat4 CollisionObject::getTransformMatrix() const {
    updateTransformCache();
    return transformMatrix;
}

glm::mat4 CollisionObject::getInverseTransformMatrix() const {
    updateTransformCache();
    return inverseTransformMatrix;
}

glm::vec3 CollisionObject::getWorldMin() const {
    if (!meshLoaded) {
        return glm::vec3(0.0f);
    }
    
    // Get local bounds
    glm::vec3 localMin = mesh.getMin();
    glm::vec3 localMax = mesh.getMax();
    
    // Transform all 8 corners of the bounding box
    glm::mat4 transform = getTransformMatrix();
    
    glm::vec3 corners[8] = {
        glm::vec3(localMin.x, localMin.y, localMin.z),
        glm::vec3(localMax.x, localMin.y, localMin.z),
        glm::vec3(localMin.x, localMax.y, localMin.z),
        glm::vec3(localMax.x, localMax.y, localMin.z),
        glm::vec3(localMin.x, localMin.y, localMax.z),
        glm::vec3(localMax.x, localMin.y, localMax.z),
        glm::vec3(localMin.x, localMax.y, localMax.z),
        glm::vec3(localMax.x, localMax.y, localMax.z)
    };
    
    glm::vec3 worldMin = glm::vec3(transform * glm::vec4(corners[0], 1.0f));
    
    for (int i = 1; i < 8; ++i) {
        glm::vec3 worldCorner = glm::vec3(transform * glm::vec4(corners[i], 1.0f));
        worldMin = glm::min(worldMin, worldCorner);
    }
    
    return worldMin;
}

glm::vec3 CollisionObject::getWorldMax() const {
    if (!meshLoaded) {
        return glm::vec3(0.0f);
    }
    
    // Get local bounds
    glm::vec3 localMin = mesh.getMin();
    glm::vec3 localMax = mesh.getMax();
    
    // Transform all 8 corners of the bounding box
    glm::mat4 transform = getTransformMatrix();
    
    glm::vec3 corners[8] = {
        glm::vec3(localMin.x, localMin.y, localMin.z),
        glm::vec3(localMax.x, localMin.y, localMin.z),
        glm::vec3(localMin.x, localMax.y, localMin.z),
        glm::vec3(localMax.x, localMax.y, localMin.z),
        glm::vec3(localMin.x, localMin.y, localMax.z),
        glm::vec3(localMax.x, localMin.y, localMax.z),
        glm::vec3(localMin.x, localMax.y, localMax.z),
        glm::vec3(localMax.x, localMax.y, localMax.z)
    };
    
    glm::vec3 worldMax = glm::vec3(transform * glm::vec4(corners[0], 1.0f));
    
    for (int i = 1; i < 8; ++i) {
        glm::vec3 worldCorner = glm::vec3(transform * glm::vec4(corners[i], 1.0f));
        worldMax = glm::max(worldMax, worldCorner);
    }
    
    return worldMax;
}

void CollisionObject::updateTransformCache() const {
    if (!transformDirty) {
        return;
    }
    
    // Build transform matrix: T * R * S
    glm::mat4 translation = glm::translate(glm::mat4(1.0f), position);
    glm::mat4 rotationMatrix = glm::mat4_cast(rotation);
    glm::mat4 scaleMatrix = glm::scale(glm::mat4(1.0f), scale);
    
    transformMatrix = translation * rotationMatrix * scaleMatrix;
    inverseTransformMatrix = glm::inverse(transformMatrix);
    
    transformDirty = false;
}

glm::vec3 CollisionObject::worldToLocal(const glm::vec3& worldPos) const {
    glm::mat4 invTransform = getInverseTransformMatrix();
    glm::vec4 localPos4 = invTransform * glm::vec4(worldPos, 1.0f);
    return glm::vec3(localPos4);
}

glm::vec3 CollisionObject::localToWorld(const glm::vec3& localPos) const {
    glm::mat4 transform = getTransformMatrix();
    glm::vec4 worldPos4 = transform * glm::vec4(localPos, 1.0f);
    return glm::vec3(worldPos4);
}

glm::vec3 CollisionObject::transformNormal(const glm::vec3& localNormal) const {
    // For normals, we use the inverse transpose of the transformation matrix
    glm::mat3 normalMatrix = glm::mat3(glm::transpose(getInverseTransformMatrix()));
    glm::vec3 worldNormal = normalMatrix * localNormal;
    return glm::normalize(worldNormal);
}
