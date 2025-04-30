#pragma once

#include <glm/glm.hpp>
#include <vector>

struct Material
{
    glm::vec3 Albedo{ 1.0f };
    float Roughness = 1.0f;
    float Metallic = 0.0f;
    glm::vec3 EmissionColor{ 0.0f };
    float EmissionPower = 0.0f;

    float ReflectionStrength = 0.5f;
    glm::vec3 ReflectionTint{ 1.0f };

    float Transparency = 0.0f;         // 0.0 = opaque, 1.0 = fully transparent
    float IndexOfRefraction = 1.5f;    // 1.0 = air, 1.33 = water, 1.5 = glass, 2.4 = diamond

    glm::vec3 GetEmission() const { return EmissionColor * EmissionPower; }
};

// Base shape interface
struct IShape
{
    virtual int GetMaterialIndex() const = 0;
};

struct Sphere : public IShape
{
    glm::vec3 Position{ 0.0f };
    float Radius = 0.5f;
    int MaterialIndex = 0;

    int GetMaterialIndex() const override { return MaterialIndex; }
};

struct Plane : public IShape
{
    glm::vec3 Normal{ 0.0f, 1.0f, 0.0f }; // Default to upward-facing plane
    float Distance = 0.0f;               // Distance from origin along normal
    int MaterialIndex = 0;

    int GetMaterialIndex() const override { return MaterialIndex; }
};

struct Box : public IShape
{
    glm::vec3 Min{ -0.5f }; // Minimum corner point
    glm::vec3 Max{ 0.5f };  // Maximum corner point
    int MaterialIndex = 0;

    int GetMaterialIndex() const override { return MaterialIndex; }
};

struct Triangle : public IShape
{
    glm::vec3 v0, v1, v2;    // Vertices
    glm::vec3 n0, n1, n2;    // Vertex normals
    int MaterialIndex = 0;

    Triangle() = default;

    Triangle(const glm::vec3& _v0, const glm::vec3& _v1, const glm::vec3& _v2)
        : v0(_v0), v1(_v1), v2(_v2)
    {
        // Calculate face normal
        glm::vec3 edge1 = v1 - v0;
        glm::vec3 edge2 = v2 - v0;
        glm::vec3 normal = glm::normalize(glm::cross(edge1, edge2));
        n0 = n1 = n2 = normal;
    }

    int GetMaterialIndex() const override { return MaterialIndex; }
};

// Add a ShapeType enum for identifying shape types
enum class ShapeType
{
    None = -1,
    Sphere = 0,
    Plane = 1,
    Box = 2,
    Triangle = 3
};

struct Scene
{
    std::vector<Sphere> Spheres;
    std::vector<Plane> Planes;
    std::vector<Box> Boxes;
    std::vector<Triangle> Triangles;
    std::vector<Material> Materials;
};