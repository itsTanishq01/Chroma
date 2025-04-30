#include "Renderer.h"
#include "Utils.h"

#include "Walnut/Random.h"

#include <execution>

void Renderer::OnResize(uint32_t width, uint32_t height)
{
    if (m_FinalImage)
    {
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
    glm::vec3 finalColor(0.0f);

    uint32_t baseSeed = x + y * m_FinalImage->GetWidth();
    baseSeed *= m_FrameIndex;

    for (int sample = 0; sample < m_Settings.SamplesPerPixel; sample++)
    {
        uint32_t seed = baseSeed + sample * 719393;

        Ray ray;
        ray.Origin = m_ActiveCamera->GetPosition();

        if (m_Settings.SamplesPerPixel > 1)
        {
            float offsetX = Utils::RandomFloat(seed) - 0.5f;
            float offsetY = Utils::RandomFloat(seed) - 0.5f;

            float ndcX = (((float)x + offsetX) / (float)m_FinalImage->GetWidth()) * 2.0f - 1.0f;
            float ndcY = (((float)y + offsetY) / (float)m_FinalImage->GetHeight()) * 2.0f - 1.0f;

            glm::vec4 target = m_ActiveCamera->GetInverseProjection() * glm::vec4(ndcX, ndcY, 1, 1);
            ray.Direction = glm::normalize(glm::vec3(m_ActiveCamera->GetInverseView() *
                glm::vec4(glm::normalize(glm::vec3(target) / target.w), 0)));
        }
        else
        {
            ray.Direction = m_ActiveCamera->GetRayDirections()[x + y * m_FinalImage->GetWidth()];
        }

        glm::vec3 light(0.0f);
        glm::vec3 contribution(1.0f);

        int bounces = 5;
        for (int i = 0; i < bounces; i++)
        {
            seed += i;

            Renderer::HitPayload payload = TraceRay(ray);
            if (payload.HitDistance < 0.0f)
            {
                glm::vec3 skyColor = glm::vec3(0.6f, 0.7f, 0.9f);
                light += skyColor * contribution;
                break;
            }

            const Material& material = m_ActiveScene->Materials[payload.ObjectIndex];
            light += material.GetEmission() * contribution;

            glm::vec3 worldPosition = payload.WorldPosition;
            glm::vec3 worldNormal = payload.WorldNormal;
            ray.Origin = worldPosition + worldNormal * 0.0001f;

            if (material.Transparency > 0.0f)
            {
                float cosTheta = glm::min(glm::dot(-ray.Direction, worldNormal), 1.0f);
                float reflectance = CalculateFresnel(cosTheta, material.IndexOfRefraction);

                reflectance = reflectance + material.ReflectionStrength * (1.0f - reflectance);

                if (Utils::RandomFloat(seed) < reflectance)
                {
                    ray.Direction = glm::reflect(ray.Direction,
                        worldNormal + material.Roughness * Utils::InUnitSphere(seed));
                    contribution *= material.ReflectionTint;
                }
                else
                {
                    float eta = 1.0f / material.IndexOfRefraction;
                    if (glm::dot(worldNormal, ray.Direction) > 0.0f)
                    {
                        worldNormal = -worldNormal;
                        eta = material.IndexOfRefraction;
                    }

                    float cosI = glm::dot(-ray.Direction, worldNormal);
                    float sinT2 = eta * eta * (1.0f - cosI * cosI);

                    if (sinT2 < 1.0f)
                    {
                        float cosT = glm::sqrt(1.0f - sinT2);
                        ray.Direction = eta * ray.Direction + (eta * cosI - cosT) * worldNormal;
                        ray.Direction = glm::normalize(ray.Direction);
                        contribution *= glm::mix(glm::vec3(1.0f), material.Albedo, material.Transparency);
                    }
                    else
                    {
                        ray.Direction = glm::reflect(ray.Direction, worldNormal);
                        contribution *= material.ReflectionTint;
                    }
                }
            }
            else
            {
                if (Utils::RandomFloat(seed) < material.ReflectionStrength * material.Metallic)
                {
                    ray.Direction = glm::reflect(ray.Direction,
                        worldNormal + material.Roughness * Utils::InUnitSphere(seed));
                    contribution *= material.Albedo * material.ReflectionTint;
                }
                else
                {
                    if (m_Settings.SlowRandom) {
                        ray.Direction = glm::normalize(worldNormal + Walnut::Random::InUnitSphere());
                    }
                    else {
                        ray.Direction = glm::normalize(worldNormal + Utils::InUnitSphere(seed));
                    }
                    contribution *= material.Albedo;
                }
            }

            if (glm::length(contribution) < 0.001f)
                break;
        }

        finalColor += light;
    }

    finalColor /= (float)m_Settings.SamplesPerPixel;
    return glm::vec4(finalColor, 1.0f);
}

float Renderer::CalculateFresnel(float cosTheta, float ior)
{
    float R0 = ((1.0f - ior) / (1.0f + ior)) * ((1.0f - ior) / (1.0f + ior));
    return R0 + (1.0f - R0) * pow(1.0f - cosTheta, 5.0f);
}

bool Renderer::ShouldReflect(const Material& material, uint32_t& seed)
{
    float reflectProb = material.Metallic * 0.5f + material.ReflectionStrength * 0.5f;
    return Utils::RandomFloat(seed) < reflectProb;
}

bool Renderer::IntersectSphere(const Ray& ray, const Sphere& sphere, float& hitDistance) const
{
    glm::vec3 origin = ray.Origin - sphere.Position;

    float a = glm::dot(ray.Direction, ray.Direction);
    float b = 2.0f * glm::dot(origin, ray.Direction);
    float c = glm::dot(origin, origin) - sphere.Radius * sphere.Radius;

    float discriminant = b * b - 4.0f * a * c;
    if (discriminant < 0.0f)
        return false;

    float closestT = (-b - glm::sqrt(discriminant)) / (2.0f * a);
    if (closestT < 0.0001f)
        return false;

    hitDistance = closestT;
    return true;
}

bool Renderer::IntersectPlane(const Ray& ray, const Plane& plane, float& hitDistance) const
{
    float denom = glm::dot(ray.Direction, plane.Normal);

    if (std::abs(denom) < 0.0001f)
        return false;

    float t = -(glm::dot(ray.Origin, plane.Normal) + plane.Distance) / denom;

    if (t < 0.0001f)
        return false;

    hitDistance = t;
    return true;
}

bool Renderer::IntersectBox(const Ray& ray, const Box& box, float& hitDistance) const
{
    glm::vec3 invDir = 1.0f / ray.Direction;
    glm::vec3 tMin = (box.Min - ray.Origin) * invDir;
    glm::vec3 tMax = (box.Max - ray.Origin) * invDir;

    if (ray.Direction.x == 0.0f) {
        tMin.x = -std::numeric_limits<float>::infinity();
        tMax.x = std::numeric_limits<float>::infinity();
    }
    if (ray.Direction.y == 0.0f) {
        tMin.y = -std::numeric_limits<float>::infinity();
        tMax.y = std::numeric_limits<float>::infinity();
    }
    if (ray.Direction.z == 0.0f) {
        tMin.z = -std::numeric_limits<float>::infinity();
        tMax.z = std::numeric_limits<float>::infinity();
    }

    if (tMin.x > tMax.x) std::swap(tMin.x, tMax.x);
    if (tMin.y > tMax.y) std::swap(tMin.y, tMax.y);
    if (tMin.z > tMax.z) std::swap(tMin.z, tMax.z);

    float tNear = std::max(std::max(tMin.x, tMin.y), tMin.z);
    float tFar = std::min(std::min(tMax.x, tMax.y), tMax.z);

    if (tNear > tFar || tFar < 0.0001f)
        return false;

    hitDistance = tNear > 0.0001f ? tNear : tFar;
    return true;
}

bool Renderer::IntersectTriangle(const Ray& ray, const Triangle& triangle, float& hitDistance, glm::vec3& normal) const
{
    glm::vec3 edge1 = triangle.v1 - triangle.v0;
    glm::vec3 edge2 = triangle.v2 - triangle.v0;
    glm::vec3 pvec = glm::cross(ray.Direction, edge2);

    float det = glm::dot(edge1, pvec);

    if (std::abs(det) < 0.0001f)
        return false;

    float invDet = 1.0f / det;

    glm::vec3 tvec = ray.Origin - triangle.v0;
    float u = glm::dot(tvec, pvec) * invDet;

    if (u < 0.0f || u > 1.0f)
        return false;

    glm::vec3 qvec = glm::cross(tvec, edge1);
    float v = glm::dot(ray.Direction, qvec) * invDet;

    if (v < 0.0f || u + v > 1.0f)
        return false;

    float t = glm::dot(edge2, qvec) * invDet;

    if (t < 0.0001f)
        return false;

    hitDistance = t;

    float w = 1.0f - u - v;
    normal = glm::normalize(w * triangle.n0 + u * triangle.n1 + v * triangle.n2);

    return true;
}

Renderer::HitPayload Renderer::TraceRay(const Ray& ray)
{
    int closestShape = -1;
    float hitDistance = std::numeric_limits<float>::max();
    ShapeType shapeType = ShapeType::None;
    glm::vec3 triangleNormal;

    for (size_t i = 0; i < m_ActiveScene->Spheres.size(); i++)
    {
        float t;
        if (IntersectSphere(ray, m_ActiveScene->Spheres[i], t) && t < hitDistance)
        {
            hitDistance = t;
            closestShape = (int)i;
            shapeType = ShapeType::Sphere;
        }
    }

    for (size_t i = 0; i < m_ActiveScene->Planes.size(); i++)
    {
        float t;
        if (IntersectPlane(ray, m_ActiveScene->Planes[i], t) && t < hitDistance)
        {
            hitDistance = t;
            closestShape = (int)i;
            shapeType = ShapeType::Plane;
        }
    }

    for (size_t i = 0; i < m_ActiveScene->Boxes.size(); i++)
    {
        float t;
        if (IntersectBox(ray, m_ActiveScene->Boxes[i], t) && t < hitDistance)
        {
            hitDistance = t;
            closestShape = (int)i;
            shapeType = ShapeType::Box;
        }
    }

    for (size_t i = 0; i < m_ActiveScene->Triangles.size(); i++)
    {
        float t;
        glm::vec3 normal;
        if (IntersectTriangle(ray, m_ActiveScene->Triangles[i], t, normal) && t < hitDistance)
        {
            hitDistance = t;
            closestShape = (int)i;
            shapeType = ShapeType::Triangle;
            triangleNormal = normal;
        }
    }

    if (closestShape < 0)
        return Miss(ray);

    return ClosestHit(ray, hitDistance, closestShape, shapeType);
}

Renderer::HitPayload Renderer::ClosestHit(const Ray& ray, float hitDistance, int objectIndex, ShapeType type)
{
    Renderer::HitPayload payload;
    payload.HitDistance = hitDistance;
    payload.ObjectIndex = objectIndex;
    payload.Type = type;

    switch (type)
    {
    case ShapeType::Sphere:
    {
        const Sphere& sphere = m_ActiveScene->Spheres[objectIndex];
        glm::vec3 origin = ray.Origin - sphere.Position;
        payload.WorldPosition = origin + ray.Direction * hitDistance;
        payload.WorldNormal = glm::normalize(payload.WorldPosition);
        payload.WorldPosition += sphere.Position;
        payload.ObjectIndex = sphere.MaterialIndex;
        break;
    }
    case ShapeType::Plane:
    {
        const Plane& plane = m_ActiveScene->Planes[objectIndex];
        payload.WorldPosition = ray.Origin + ray.Direction * hitDistance;
        payload.WorldNormal = plane.Normal;
        payload.ObjectIndex = plane.MaterialIndex;
        break;
    }
    case ShapeType::Box:
    {
        const Box& box = m_ActiveScene->Boxes[objectIndex];
        payload.WorldPosition = ray.Origin + ray.Direction * hitDistance;

        glm::vec3 p = payload.WorldPosition;
        float epsilon = 0.0001f;

        if (std::abs(p.x - box.Min.x) < epsilon)
            payload.WorldNormal = glm::vec3(-1, 0, 0);
        else if (std::abs(p.x - box.Max.x) < epsilon)
            payload.WorldNormal = glm::vec3(1, 0, 0);
        else if (std::abs(p.y - box.Min.y) < epsilon)
            payload.WorldNormal = glm::vec3(0, -1, 0);
        else if (std::abs(p.y - box.Max.y) < epsilon)
            payload.WorldNormal = glm::vec3(0, 1, 0);
        else if (std::abs(p.z - box.Min.z) < epsilon)
            payload.WorldNormal = glm::vec3(0, 0, -1);
        else if (std::abs(p.z - box.Max.z) < epsilon)
            payload.WorldNormal = glm::vec3(0, 0, 1);

        payload.ObjectIndex = box.MaterialIndex;
        break;
    }
    case ShapeType::Triangle:
    {
        const Triangle& triangle = m_ActiveScene->Triangles[objectIndex];
        payload.WorldPosition = ray.Origin + ray.Direction * hitDistance;
        payload.WorldNormal = triangle.n0;
        payload.ObjectIndex = triangle.MaterialIndex;
        break;
    }
    }

    return payload;
}

Renderer::HitPayload Renderer::Miss(const Ray& ray)
{
    Renderer::HitPayload payload;
    payload.HitDistance = -1.0f;
    return payload;
}