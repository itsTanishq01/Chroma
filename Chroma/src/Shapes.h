#pragma once

#include "Scene.h"
#include <glm/glm.hpp>

namespace Shapes {

    // Add a pyramid shape to the scene
    inline void AddPyramid(Scene& scene, const glm::vec3& baseCenter, float baseSize, float height, int materialIndex) {
        // Calculate the base corners
        float halfSize = baseSize * 0.5f;
        glm::vec3 baseTopLeft(baseCenter.x - halfSize, baseCenter.y, baseCenter.z - halfSize);
        glm::vec3 baseTopRight(baseCenter.x + halfSize, baseCenter.y, baseCenter.z - halfSize);
        glm::vec3 baseBottomLeft(baseCenter.x - halfSize, baseCenter.y, baseCenter.z + halfSize);
        glm::vec3 baseBottomRight(baseCenter.x + halfSize, baseCenter.y, baseCenter.z + halfSize);
        glm::vec3 apex(baseCenter.x, baseCenter.y + height, baseCenter.z);

        // Base (two triangles)
        Triangle baseTriangle1(baseBottomLeft, baseTopLeft, baseTopRight);
        baseTriangle1.MaterialIndex = materialIndex;
        scene.Triangles.push_back(baseTriangle1);

        Triangle baseTriangle2(baseBottomLeft, baseTopRight, baseBottomRight);
        baseTriangle2.MaterialIndex = materialIndex;
        scene.Triangles.push_back(baseTriangle2);

        // Four triangular faces
        Triangle frontFace(baseBottomLeft, baseBottomRight, apex);
        frontFace.MaterialIndex = materialIndex;
        scene.Triangles.push_back(frontFace);

        Triangle rightFace(baseBottomRight, baseTopRight, apex);
        rightFace.MaterialIndex = materialIndex;
        scene.Triangles.push_back(rightFace);

        Triangle backFace(baseTopRight, baseTopLeft, apex);
        backFace.MaterialIndex = materialIndex;
        scene.Triangles.push_back(backFace);

        Triangle leftFace(baseTopLeft, baseBottomLeft, apex);
        leftFace.MaterialIndex = materialIndex;
        scene.Triangles.push_back(leftFace);
    }

    // Add a cube shape to the scene
    inline void AddCube(Scene& scene, const glm::vec3& center, float size, int materialIndex) {
        Box box;
        float halfSize = size * 0.5f;
        box.Min = center - glm::vec3(halfSize);
        box.Max = center + glm::vec3(halfSize);
        box.MaterialIndex = materialIndex;
        scene.Boxes.push_back(box);
    }

    // Add a sphere shape to the scene
    inline void AddSphere(Scene& scene, const glm::vec3& center, float radius, int materialIndex) {
        Sphere sphere;
        sphere.Position = center;
        sphere.Radius = radius;
        sphere.MaterialIndex = materialIndex;
        scene.Spheres.push_back(sphere);
    }

    // Add a plane shape to the scene
    inline void AddPlane(Scene& scene, const glm::vec3& normal, float distance, int materialIndex) {
        Plane plane;
        plane.Normal = glm::normalize(normal); // Ensure normal is unit vector
        plane.Distance = distance;
        plane.MaterialIndex = materialIndex;
        scene.Planes.push_back(plane);
    }
}