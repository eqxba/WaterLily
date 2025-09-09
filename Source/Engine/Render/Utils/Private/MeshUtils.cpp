#include "Engine/Render/Utils/MeshUtils.hpp"

std::pair<std::vector<glm::vec3>, std::vector<uint32_t>> MeshUtils::GenerateSphereMesh(const uint32_t stacks, const uint32_t sectors)
{
    using namespace std::numbers;
    
    std::vector<glm::vec3> vertices;
    std::vector<uint32_t> indices;
    
    for (uint32_t i = 0; i <= stacks; ++i)
    {
        float stackAngle = pi / 2.0f - i * (pi / stacks);
        float xy = cosf(stackAngle);
        float z  = sinf(stackAngle);

        for (uint32_t j = 0; j <= sectors; ++j)
        {
            float sectorAngle = j * (2.0f * pi / sectors);
            float x = xy * cosf(sectorAngle);
            float y = xy * sinf(sectorAngle);
            
            vertices.emplace_back(x, y, z);
        }
    }

    for (uint32_t i = 0; i < stacks; ++i)
    {
        for (uint32_t j = 0; j < sectors; ++j)
        {
            uint32_t first = i * (sectors + 1) + j;
            uint32_t second = first + sectors + 1;

            indices.push_back(first);
            indices.push_back(second);
            indices.push_back(first + 1);

            indices.push_back(second);
            indices.push_back(second + 1);
            indices.push_back(first + 1);
        }
    }

    return { vertices, indices };
}

std::vector<glm::vec3> MeshUtils::GenerateCircleLineStrip(const uint32_t segments)
{
    using namespace std::numbers;
    
    std::vector<glm::vec3> vertices;
    vertices.reserve(segments + 1);

    const float step = 2.0f * pi / static_cast<float>(segments);

    for (uint32_t i = 0; i < segments; i++)
    {
        const float angle = i * step;
        vertices.emplace_back(cos(angle), sin(angle), 0.0f);
    }

    vertices.push_back(vertices[0]);

    return vertices;
}

std::vector<glm::vec3> MeshUtils::GenerateCircleLineList(const uint32_t segments)
{
    using namespace std::numbers;

    std::vector<glm::vec3> vertices;
    vertices.reserve(segments * 2);

    const float step = 2.0f * pi / static_cast<float>(segments);

    for (uint32_t i = 0; i < segments - 1; i++)
    {
        const float angle0 = i * step;
        const float angle1 = (i + 1) * step;
        
        vertices.emplace_back(cos(angle0), sin(angle0), 0.0f);
        vertices.emplace_back(cos(angle1), sin(angle1), 0.0f);
    }
    
    vertices.push_back(vertices.back());
    vertices.push_back(vertices.front());

    return vertices;
}

std::array<glm::vec3, 8> MeshUtils::GenerateFrustumCorners(const CameraComponent& camera)
{
    constexpr float farZ = 500.0f;
    
    std::array<glm::vec3, 8> corners;
    
    const float near = camera.GetNear();
    const float verticalFov = camera.GetVerticalFov();
    const float aspectRatio = camera.GetAspectRatio();
    
    const auto nearCenter = glm::vec3(0.0f, 0.0f, -near);
    const auto farCenter = glm::vec3(0.0f, 0.0f, -farZ);
    
    const float heightNear = 2.0f * glm::tan(verticalFov * 0.5f) * near;
    const float widthNear = heightNear * aspectRatio;

    const float heightFar = 2.0f * glm::tan(verticalFov  * 0.5f) * farZ;
    const float widthFar = heightFar * aspectRatio;
    
    corners[0] = nearCenter + glm::vec3(-widthNear / 2.0f,  heightNear / 2.0f, 0.0f); // near top-left
    corners[1] = nearCenter + glm::vec3( widthNear / 2.0f,  heightNear / 2.0f, 0.0f); // near top-right
    corners[2] = nearCenter + glm::vec3( widthNear / 2.0f, -heightNear / 2.0f, 0.0f); // near bottom-right
    corners[3] = nearCenter + glm::vec3(-widthNear / 2.0f, -heightNear / 2.0f, 0.0f); // near bottom-left
    
    corners[4] = farCenter + glm::vec3(-widthFar / 2.0f,  heightFar / 2.0f, 0.0f); // far top-left
    corners[5] = farCenter + glm::vec3( widthFar / 2.0f,  heightFar / 2.0f, 0.0f); // far top-right
    corners[6] = farCenter + glm::vec3( widthFar / 2.0f, -heightFar / 2.0f, 0.0f); // far bottom-right
    corners[7] = farCenter + glm::vec3(-widthFar / 2.0f, -heightFar / 2.0f, 0.0f); // far bottom-left
    
    const glm::mat4 invView = glm::inverse(camera.GetViewMatrix());
    
    std::ranges::for_each(corners, [&](glm::vec3& corner) {
        corner = glm::vec3(invView * glm::vec4(corner, 1.0f));
    });
    
    return corners;
}
