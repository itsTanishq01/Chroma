#include "Renderer.h"

#include "Walnut/Random.h"

#include <execution>

namespace Utils {

	static uint32_t ConvertToRGBA(const glm::vec4& color)
	{
		uint8_t r = (uint8_t)(color.r * 255.0f);
		uint8_t g = (uint8_t)(color.g * 255.0f);    
		uint8_t b = (uint8_t)(color.b * 255.0f);
		uint8_t a = (uint8_t)(color.a * 255.0f);

		uint32_t result = (a << 24) | (b << 16) | (g << 8) | r;
		return result;
	}
	static uint32_t PCG_Hash(uint32_t input)
	{
		uint32_t state = input * 747796405u + 2891336453u;
		uint32_t word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
		return ((word >> 22u) ^ word);
	}

	static float RandomFloat(uint32_t& seed)
	{
		seed = PCG_Hash(seed);
		return (float)seed / (float)std::numeric_limits<uint32_t>::max();

        //return Walnut::Random::Float();
	}

	static glm::vec3 InUnitSphere(uint32_t& seed)
	{
		return glm::normalize(glm::vec3(
		    RandomFloat(seed) * 2.0f - 1.0f, 
		    RandomFloat(seed) * 2.0f - 1.0f,
	        RandomFloat(seed) * 2.0f - 1.0f));

        //return Walnut::Random::InUnitSphere();

	}
}

void Renderer::OnResize(uint32_t width, uint32_t height)
{
	if (m_FinalImage)
	{
		// No resize necessary
		if (m_FinalImage->GetWidth() == width && m_FinalImage->GetHeight() == height)
			return;

		m_FinalImage->Resize(width, height);
	}
	else
	{
		m_FinalImage = std::make_shared<Walnut::Image>(width, height, Walnut::ImageFormat::RGBA);
	}

	delete[] m_ImageData;
	m_ImageData = new uint32_t[width * height];

	delete[] m_AccumulationData;
	m_AccumulationData = new glm::vec4[width * height];

	m_ImageHorizontalIter.resize(width);
	m_ImageVerticalIter.resize(height);
	for (uint32_t i = 0; i < width; i++)
		m_ImageHorizontalIter[i] = i;
	for (uint32_t i = 0; i < height; i++)
		m_ImageVerticalIter[i] = i;
}

void Renderer::Render(const Scene& scene, const Camera& camera)
{
	m_ActiveScene = &scene;
	m_ActiveCamera = &camera;

	if (m_FrameIndex == 1)
		memset(m_AccumulationData, 0, m_FinalImage->GetWidth() * m_FinalImage->GetHeight() * sizeof(glm::vec4));

#define MT 1
#if MT
	std::for_each(std::execution::par, m_ImageVerticalIter.begin(), m_ImageVerticalIter.end(),
		[this](uint32_t y)
		{
			std::for_each(std::execution::par, m_ImageHorizontalIter.begin(), m_ImageHorizontalIter.end(),
				[this, y](uint32_t x)
				{
					glm::vec4 color = PerPixel(x, y);
					m_AccumulationData[x + y * m_FinalImage->GetWidth()] += color;

					glm::vec4 accumulatedColor = m_AccumulationData[x + y * m_FinalImage->GetWidth()];
					accumulatedColor /= (float)m_FrameIndex;

					accumulatedColor = glm::clamp(accumulatedColor, glm::vec4(0.0f), glm::vec4(1.0f));
					m_ImageData[x + y * m_FinalImage->GetWidth()] = Utils::ConvertToRGBA(accumulatedColor);
				});
		});

#else

	for (uint32_t y = 0; y < m_FinalImage->GetHeight(); y++)
	{
		for (uint32_t x = 0; x < m_FinalImage->GetWidth(); x++)
		{
			glm::vec4 color = PerPixel(x, y);
			m_AccumulationData[x + y * m_FinalImage->GetWidth()] += color;

			glm::vec4 accumulatedColor = m_AccumulationData[x + y * m_FinalImage->GetWidth()];
			accumulatedColor /= (float)m_FrameIndex;

			accumulatedColor = glm::clamp(accumulatedColor, glm::vec4(0.0f), glm::vec4(1.0f));
			m_ImageData[x + y * m_FinalImage->GetWidth()] = Utils::ConvertToRGBA(accumulatedColor);
		}
	}
#endif

	m_FinalImage->SetData(m_ImageData);

	if (m_Settings.Accumulate)
		m_FrameIndex++;
	else
		m_FrameIndex = 1;
}

glm::vec4 Renderer::PerPixel(uint32_t x, uint32_t y)
{
    // Final accumulated color for this pixel
    glm::vec3 finalColor(0.0f);

    // Base seed for this pixel
    uint32_t baseSeed = x + y * m_FinalImage->GetWidth();
    baseSeed *= m_FrameIndex;

    // Sample multiple rays per pixel
    for (int sample = 0; sample < m_Settings.SamplesPerPixel; sample++)
    {
        // Create a different seed for each sample
        uint32_t seed = baseSeed + sample * 719393; // Use a prime number for better distribution

        // Ray for this sample
        Ray ray;
        ray.Origin = m_ActiveCamera->GetPosition();

        if (m_Settings.SamplesPerPixel > 1)
        {
            // Calculate sub-pixel offsets for anti-aliasing
            float offsetX = Utils::RandomFloat(seed) - 0.5f;
            float offsetY = Utils::RandomFloat(seed) - 0.5f;

            // Convert pixel coordinates to normalized device coordinates (-1 to 1)
            float ndcX = (((float)x + offsetX) / (float)m_FinalImage->GetWidth()) * 2.0f - 1.0f;
            float ndcY = (((float)y + offsetY) / (float)m_FinalImage->GetHeight()) * 2.0f - 1.0f;

            // Create ray direction based on jittered position
            glm::vec4 target = m_ActiveCamera->GetInverseProjection() * glm::vec4(ndcX, ndcY, 1, 1);
            ray.Direction = glm::normalize(glm::vec3(m_ActiveCamera->GetInverseView() *
                glm::vec4(glm::normalize(glm::vec3(target) / target.w), 0)));
        }
        else
        {
            // Use original ray direction when not anti-aliasing
            ray.Direction = m_ActiveCamera->GetRayDirections()[x + y * m_FinalImage->GetWidth()];
        }

        // Trace the ray
        glm::vec3 light(0.0f);
        glm::vec3 contribution(1.0f);

        int bounces = 5;
        for (int i = 0; i < bounces; i++)
        {
            seed += i;

            Renderer::HitPayload payload = TraceRay(ray);
            if (payload.HitDistance < 0.0f)
            {
                // Set background/sky color
                glm::vec3 skyColor = glm::vec3(0.6f, 0.7f, 0.9f);
                light += skyColor * contribution;
                break;
            }

            const Sphere& sphere = m_ActiveScene->Spheres[payload.ObjectIndex];
            const Material& material = m_ActiveScene->Materials[sphere.MaterialIndex];

            // Add emission contribution
            light += material.GetEmission() * contribution;

            // Calculate surface data
            glm::vec3 worldPosition = payload.WorldPosition;
            glm::vec3 worldNormal = payload.WorldNormal;

            // Offset ray origin slightly to prevent self-intersection
            ray.Origin = worldPosition + worldNormal * 0.0001f;

            // Check for transparency
            if (material.Transparency > 0.0f)
            {
                // Calculate whether we refract or reflect (based on Fresnel)
                float cosTheta = glm::min(glm::dot(-ray.Direction, worldNormal), 1.0f);
                float reflectance = CalculateFresnel(cosTheta, material.IndexOfRefraction);

                // Mix reflectance with material's reflection strength
                reflectance = reflectance + material.ReflectionStrength * (1.0f - reflectance);

                // Random choice between reflection and refraction based on Fresnel
                if (Utils::RandomFloat(seed) < reflectance)
                {
                    // Reflection ray
                    ray.Direction = glm::reflect(ray.Direction,
                        worldNormal + material.Roughness * Utils::InUnitSphere(seed));

                    // Apply reflection tint
                    contribution *= material.ReflectionTint;
                }
                else
                {
                    // Refraction ray
                    float eta = 1.0f / material.IndexOfRefraction;
                    if (glm::dot(worldNormal, ray.Direction) > 0.0f)
                    {
                        // We're exiting the object
                        worldNormal = -worldNormal;
                        eta = material.IndexOfRefraction;
                    }

                    // Use Snell's law for refraction
                    float cosI = glm::dot(-ray.Direction, worldNormal);
                    float sinT2 = eta * eta * (1.0f - cosI * cosI);

                    if (sinT2 < 1.0f) // Check for total internal reflection
                    {
                        float cosT = glm::sqrt(1.0f - sinT2);
                        ray.Direction = eta * ray.Direction + (eta * cosI - cosT) * worldNormal;
                        ray.Direction = glm::normalize(ray.Direction);

                        // Apply transparency tint
                        contribution *= glm::mix(glm::vec3(1.0f), material.Albedo, material.Transparency);
                    }
                    else
                    {
                        // Total internal reflection
                        ray.Direction = glm::reflect(ray.Direction, worldNormal);
                        contribution *= material.ReflectionTint;
                    }
                }
            }
            else
            {
                // Regular surface (diffuse + specular)

                // Use reflection strength to determine if we reflect or diffuse
                if (Utils::RandomFloat(seed) < material.ReflectionStrength * material.Metallic)
                {
                    // Reflection ray (apply roughness for blurry reflections)
                    ray.Direction = glm::reflect(ray.Direction,
                        worldNormal + material.Roughness * Utils::InUnitSphere(seed));

                    // Apply reflection tint and metallic surface color
                    contribution *= material.Albedo * material.ReflectionTint;
                }
                else
                {
                    // Diffuse ray (random direction in hemisphere)
                    if (m_Settings.SlowRandom) {
                        ray.Direction = glm::normalize(worldNormal + Walnut::Random::InUnitSphere());
                    }
                    else {
                        ray.Direction = glm::normalize(worldNormal + Utils::InUnitSphere(seed));
                    }

                    // Apply albedo for diffuse surfaces
                    contribution *= material.Albedo;
                }
            }

            // If contribution is very small, terminate path
            if (glm::length(contribution) < 0.001f)
                break;
        }

        // Add this sample's contribution to the final color
        finalColor += light;
    }

    // Average all the samples
    finalColor /= (float)m_Settings.SamplesPerPixel;

    return glm::vec4(finalColor, 1.0f);
}

// Helper function for Fresnel calculation
float Renderer::CalculateFresnel(float cosTheta, float ior)
{
	// Schlick's approximation for Fresnel
	float R0 = ((1.0f - ior) / (1.0f + ior)) * ((1.0f - ior) / (1.0f + ior));
	return R0 + (1.0f - R0) * pow(1.0f - cosTheta, 5.0f);
}

// Helper function to determine if we should reflect
bool Renderer::ShouldReflect(const Material& material, uint32_t& seed)
{
	// Combine metallic and reflection strength to determine reflection probability
	float reflectProb = material.Metallic * 0.5f + material.ReflectionStrength * 0.5f;

	// Use random number to determine if we reflect
	return Utils::RandomFloat(seed) < reflectProb;
}

Renderer::HitPayload Renderer::TraceRay(const Ray& ray)
{
	// (bx^2 + by^2)t^2 + (2(axbx + ayby))t + (ax^2 + ay^2 - r^2) = 0
	// where
	// a = ray origin
	// b = ray direction
	// r = radius
	// t = hit distance

	int closestSphere = -1;
	float hitDistance = std::numeric_limits<float>::max();
	for (size_t i = 0; i < m_ActiveScene->Spheres.size(); i++)
	{
		const Sphere& sphere = m_ActiveScene->Spheres[i];
		glm::vec3 origin = ray.Origin - sphere.Position;

		float a = glm::dot(ray.Direction, ray.Direction);
		float b = 2.0f * glm::dot(origin, ray.Direction);
		float c = glm::dot(origin, origin) - sphere.Radius * sphere.Radius;

		// Quadratic forumula discriminant:
		// b^2 - 4ac

		float discriminant = b * b - 4.0f * a * c;
		if (discriminant < 0.0f)
			continue;

		// Quadratic formula:
		// (-b +- sqrt(discriminant)) / 2a

		// float t0 = (-b + glm::sqrt(discriminant)) / (2.0f * a); // Second hit distance (currently unused)
		float closestT = (-b - glm::sqrt(discriminant)) / (2.0f * a);
		if (closestT > 0.0f && closestT < hitDistance)
		{
			hitDistance = closestT;
			closestSphere = (int)i;
		}
	}

	if (closestSphere < 0)
		return Miss(ray);

	return ClosestHit(ray, hitDistance, closestSphere);
}

Renderer::HitPayload Renderer::ClosestHit(const Ray& ray, float hitDistance, int objectIndex)
{
	Renderer::HitPayload payload;
	payload.HitDistance = hitDistance;
	payload.ObjectIndex = objectIndex;

	const Sphere& closestSphere = m_ActiveScene->Spheres[objectIndex];

	glm::vec3 origin = ray.Origin - closestSphere.Position;
	payload.WorldPosition = origin + ray.Direction * hitDistance;
	payload.WorldNormal = glm::normalize(payload.WorldPosition);

	payload.WorldPosition += closestSphere.Position;

	return payload;
}

Renderer::HitPayload Renderer::Miss(const Ray& ray)
{
	Renderer::HitPayload payload;
	payload.HitDistance = -1.0f;
	return payload;
}