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

struct Sphere
{
	glm::vec3 Position{ 0.0f };
	float Radius = 0.5f;

	int MaterialIndex = 0;
};

struct Scene
{
	std::vector<Sphere> Spheres;
	std::vector<Material> Materials;
};