#include "Renderer.h"

#include <limits>

// Sphere intersection test
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

// Plane intersection test
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

// Box intersection test
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

// Triangle intersection test
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