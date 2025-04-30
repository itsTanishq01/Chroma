#pragma once

#include <glm/glm.hpp>
#include <limits>

namespace Utils {
    // Convert a vector4 color to RGBA format for image display
    inline uint32_t ConvertToRGBA(const glm::vec4& color)
    {
        uint8_t r = (uint8_t)(color.r * 255.0f);
        uint8_t g = (uint8_t)(color.g * 255.0f);
        uint8_t b = (uint8_t)(color.b * 255.0f);
        uint8_t a = (uint8_t)(color.a * 255.0f);

        return (a << 24) | (b << 16) | (g << 8) | r;
    }

    // PCG hash function for random number generation
    inline uint32_t PCG_Hash(uint32_t input)
    {
        uint32_t state = input * 747796405u + 2891336453u;
        uint32_t word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
        return ((word >> 22u) ^ word);
    }

    // Generate random float between 0 and 1
    inline float RandomFloat(uint32_t& seed)
    {
        seed = PCG_Hash(seed);
        return (float)seed / (float)std::numeric_limits<uint32_t>::max();
    }

    // Generate random unit vector
    inline glm::vec3 InUnitSphere(uint32_t& seed)
    {
        return glm::normalize(glm::vec3(
            RandomFloat(seed) * 2.0f - 1.0f,
            RandomFloat(seed) * 2.0f - 1.0f,
            RandomFloat(seed) * 2.0f - 1.0f));
    }
}