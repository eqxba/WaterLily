#include "Utils/Math.hpp"

#include "Utils/Constants.hpp"

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