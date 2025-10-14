#include "Utils/Math.hpp"

#include "Utils/Constants.hpp"

DISABLE_WARNINGS_BEGIN
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/norm.hpp>
#undef GLM_ENABLE_EXPERIMENTAL
DISABLE_WARNINGS_END

#include <random>

namespace MathDetails
{
    static std::mt19937 rng(std::random_device{}());

    static float Dist(const glm::vec3& a, const glm::vec3& b)
    {
        return glm::length(a - b);
    }

    static bool IsInside(const Sphere& s, const glm::vec3& p)
    {
        return Dist(s.center, p) <= s.radius + 1e-6f;
    }

    static Sphere SphereFrom(const glm::vec3& a)
    {
        return { a, 0.0f };
    }

    static Sphere SphereFrom(const glm::vec3& a, const glm::vec3& b)
    {
        return { (a + b) * 0.5f, Dist(a, b) * 0.5f };
    }

    static Sphere SphereFrom(const glm::vec3& a, const glm::vec3& b, const glm::vec3& c)
    {
        const glm::vec3 ab = b - a;
        const glm::vec3 ac = c - a;
        const glm::vec3 abXac = glm::cross(ab, ac);
        const float denom = 2.0f * glm::length2(abXac);
        
        if (denom < 1e-12f)
        {
            Sphere s = SphereFrom(a, b);
            
            if (!IsInside(s, c))
            {
                s = SphereFrom(a, c);
            }
            if (!IsInside(s, b))
            {
                s = SphereFrom(b, c);
            }
            
            return s;
        }

        const float ab2 = glm::length2(ab);
        const float ac2 = glm::length2(ac);
        const glm::vec3 o = a + (glm::cross(abXac, ab) * ac2 + glm::cross(ac, abXac) * ab2) / denom;
        const float r = Dist(o, a);
        
        return { o, r };
    }

    static Sphere SphereFrom(const glm::vec3& a, const glm::vec3& b, const glm::vec3& c, const glm::vec3& d)
    {
        // https://math.stackexchange.com/questions/213658
        const float a2 = glm::dot(a, a);
        const float b2 = glm::dot(b, b);
        const float c2 = glm::dot(c, c);
        const float d2 = glm::dot(d, d);

        const glm::mat4 m = {
            glm::vec4(a.x, a.y, a.z, 1.0f),
            glm::vec4(b.x, b.y, b.z, 1.0f),
            glm::vec4(c.x, c.y, c.z, 1.0f),
            glm::vec4(d.x, d.y, d.z, 1.0f) };
        
        const float detM = glm::determinant(m);
        
        if (std::fabs(detM) < 1e-12f)
        {
            Sphere s = SphereFrom(a, b, c);
            
            if (!IsInside(s, d))
            {
                s = SphereFrom(a, b, d);
            }
            if (!IsInside(s, c))
            {
                s = SphereFrom(a, c, d);
            }
            if (!IsInside(s, b))
            {
                s = SphereFrom(b, c, d);
            }
            
            return s;
        }

        const glm::mat4 mx = {
            glm::vec4(a2, a.y, a.z, 1.0f),
            glm::vec4(b2, b.y, b.z, 1.0f),
            glm::vec4(c2, c.y, c.z, 1.0f),
            glm::vec4(d2, d.y, d.z, 1.0f) };

        const glm::mat4 my = {
            glm::vec4(a2, a.x, a.z, 1.0f),
            glm::vec4(b2, b.x, b.z, 1.0f),
            glm::vec4(c2, c.x, c.z, 1.0f),
            glm::vec4(d2, d.x, d.z, 1.0f) };

        const glm::mat4 mz = {
            glm::vec4(a2, a.x, a.y, 1.0f),
            glm::vec4(b2, b.x, b.y, 1.0f),
            glm::vec4(c2, c.x, c.y, 1.0f),
            glm::vec4(d2, d.x, d.y, 1.0f) };

        const glm::vec3 o(0.5f * glm::determinant(mx) / detM, -0.5f * glm::determinant(my) / detM,
            0.5f * glm::determinant(mz) / detM);

        return { o, Dist(o, a) };
    }

    static bool IsValidSphere(const Sphere& s, const std::vector<glm::vec3>& points)
    {
        return std::ranges::all_of(points, [&](const glm::vec3& p) { return IsInside(s, p); });
    }

    static Sphere MinSphereTrivial(const std::vector<glm::vec3>& points)
    {
        Assert(points.size() <= 4);
        
        switch (points.size())
        {
            case 1: return SphereFrom(points[0]);
            case 2: return SphereFrom(points[0], points[1]);
            case 3: return SphereFrom(points[0], points[1], points[2]);
            case 4: return SphereFrom(points[0], points[1], points[2], points[3]);
        }
        
        return { glm::vec3(0.0f), 0.0f };
    }

    // Modified for 3D version of https://www.geeksforgeeks.org/dsa/minimum-enclosing-circle-using-welzls-algorithm/
    static Sphere WelzlRecursive(std::vector<glm::vec3>& p, std::vector<glm::vec3> r, const int n)
    {
        if (n == 0 || r.size() == 4)
        {
            return MinSphereTrivial(r);
        }
        
        std::uniform_int_distribution<int> indexDist(0, n - 1);
        const int index = indexDist(rng);

        const glm::vec3 point = p[index];
        std::swap(p[index], p[n - 1]);

        if (const Sphere s = WelzlRecursive(p, r, n - 1); IsInside(s, point))
        {
            return s;
        }

        r.push_back(point);
        
        return WelzlRecursive(p, r, n - 1);
    }
}

glm::mat4 Math::Perspective(const float verticalFov, const float aspectRatio, const float near, const float far,
    const bool reverseDepth /* = true */)
{
    glm::mat4 matrix = Matrix4::zero;

    const float tan = glm::tan(verticalFov / 2.0f);
    const float top = near * tan; // half height of near plane
    const float right = top * aspectRatio; // half width of near plane

    matrix[0][0] = near / right;
    matrix[1][1] = -near / top;
    matrix[2][2] = reverseDepth ? near / (near - far) : -far / (far - near);
    matrix[2][3] = -1.0f;
    matrix[3][2] = reverseDepth ? -(far * near) / (near - far) : -(far * near) / (far - near);

    return matrix;
}

glm::mat4 Math::PerspectiveInfinite(const float verticalFov, const float aspectRatio, const float near, 
    const bool reverseDepth /* = true */)
{
    glm::mat4 matrix = Matrix4::zero;

    const float tan = glm::tan(verticalFov / 2.0f);
    const float top = near * tan; // half height of near plane
    const float right = top * aspectRatio; // half width of near plane

    matrix[0][0] = near / right;
    matrix[1][1] = -near / top;
    matrix[2][2] = reverseDepth ? 0.0f : -1.0f;
    matrix[2][3] = -1.0f;
    matrix[3][2] = reverseDepth ? near : -near;

    return matrix;
}

Sphere Math::Welzl(std::vector<glm::vec3> points)
{
    std::ranges::shuffle(points, MathDetails::rng);

    return MathDetails::WelzlRecursive(points, {}, static_cast<int>(points.size()));
}

glm::vec4 Math::NormalizePlane(const glm::vec4 plane)
{
    return plane / glm::length(glm::vec3(plane));
}
