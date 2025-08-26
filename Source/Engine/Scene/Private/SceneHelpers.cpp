#include "Engine/Scene/SceneHelpers.hpp"

#include "Engine/EngineConfig.hpp"
#include "Utils/Helpers.hpp"

DISABLE_WARNINGS_BEGIN
#define CGLTF_IMPLEMENTATION
#include <cgltf.h>
#include <glm/gtc/type_ptr.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/norm.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#undef GLM_ENABLE_EXPERIMENTAL
DISABLE_WARNINGS_END

#include <meshoptimizer.h>
#include <random>

namespace SceneHelpersDetails
{
    static constexpr size_t positionComponents = 3;
    static constexpr size_t normalComponents = 3;
    static constexpr size_t tangentComponents = 4;
    static constexpr size_t uvComponents = 2;
    static constexpr size_t colorComponents = 4;

    static constexpr size_t vertexSize = sizeof(gpu::Vertex);

    constexpr float coneWeight = 0.0f; // 0.25 will be default when we start using cone culling

    static void LoadVertices(const cgltf_primitive& primitive, std::span<gpu::Vertex> vertices)
    {
        const size_t vertexCount = vertices.size();

        Assert(primitive.attributes[0].data->count == vertexCount);

        std::vector<float> rawData(vertexCount * 4);

        if (const cgltf_accessor* positionAccessor = cgltf_find_accessor(&primitive, cgltf_attribute_type_position, 0))
        {
            Assert(cgltf_num_components(positionAccessor->type) == positionComponents);

            cgltf_accessor_unpack_floats(positionAccessor, rawData.data(), vertexCount * positionComponents);

            for (size_t i = 0; i < vertexCount; ++i)
            {
                vertices[i].posAndU = glm::vec4(rawData[i * 3 + 0], rawData[i * 3 + 1], rawData[i * 3 + 2], 0.0f);
            }
        }

        if (const cgltf_accessor* normalAccessor = cgltf_find_accessor(&primitive, cgltf_attribute_type_normal, 0))
        {
            Assert(cgltf_num_components(normalAccessor->type) == normalComponents);

            cgltf_accessor_unpack_floats(normalAccessor, rawData.data(), vertexCount * normalComponents);

            for (size_t i = 0; i < vertexCount; ++i)
            {
                vertices[i].normalAndV = glm::vec4(rawData[i * 3 + 0], rawData[i * 3 + 1], rawData[i * 3 + 2], 0.0f);
            }
        }

        if (const cgltf_accessor* tangentAccessor = cgltf_find_accessor(&primitive, cgltf_attribute_type_tangent, 0))
        {
            Assert(cgltf_num_components(tangentAccessor->type) == tangentComponents);

            cgltf_accessor_unpack_floats(tangentAccessor, rawData.data(), vertexCount * tangentComponents);

            for (size_t i = 0; i < vertexCount; ++i)
            {
                vertices[i].tangent = glm::vec4(rawData[i * 4 + 0], rawData[i * 4 + 1], rawData[i * 4 + 2],
                    rawData[i * 4 + 3]);
            }
        }

        if (const cgltf_accessor* uvAccessor = cgltf_find_accessor(&primitive, cgltf_attribute_type_texcoord, 0))
        {
            Assert(cgltf_num_components(uvAccessor->type) == uvComponents);

            cgltf_accessor_unpack_floats(uvAccessor, rawData.data(), vertexCount * uvComponents);

            for (size_t i = 0; i < vertexCount; ++i)
            {
                vertices[i].posAndU.w = rawData[i * 2 + 0];
                vertices[i].normalAndV.w = rawData[i * 2 + 1];
            }
        }

        if (const cgltf_accessor* colorAccessor = cgltf_find_accessor(&primitive, cgltf_attribute_type_color, 0))
        {
            Assert(cgltf_num_components(colorAccessor->type) == colorComponents);

            cgltf_accessor_unpack_floats(colorAccessor, rawData.data(), vertexCount * colorComponents);

            for (size_t i = 0; i < vertexCount; ++i)
            {
                vertices[i].color = glm::vec4(rawData[i * 4 + 0], rawData[i * 4 + 1], rawData[i * 4 + 2],
                    rawData[i * 4 + 3]);
            }
        }
    }

    static void LoadIndices(const cgltf_primitive& primitive, std::span<uint32_t> indices)
    {
        Assert(primitive.indices->count == indices.size());

        cgltf_accessor_unpack_indices(primitive.indices, indices.data(), sizeof(uint32_t), indices.size());
    }

    static uint32_t OptimizePrimitive(std::span<gpu::Vertex>& vertices, std::span<uint32_t> indices)
    {
        using namespace SceneHelpersDetails;

        std::vector<uint32_t> remap(vertices.size());
        const size_t uniqueVertices = meshopt_generateVertexRemap(remap.data(), indices.data(), indices.size(),
            vertices.data(), vertices.size(), vertexSize);

        meshopt_remapVertexBuffer(vertices.data(), vertices.data(), vertices.size(), vertexSize, remap.data());
        meshopt_remapIndexBuffer(indices.data(), indices.data(), indices.size(), remap.data());

        const auto removedVertices = static_cast<uint32_t>(vertices.size() - uniqueVertices);
        vertices = vertices.first(uniqueVertices);

        meshopt_optimizeVertexCache(indices.data(), indices.data(), indices.size(), vertices.size());

        meshopt_optimizeVertexFetch(vertices.data(), indices.data(), indices.size(), vertices.data(), vertices.size(), vertexSize);

        return removedVertices;
    }

    static glm::vec3 CalculateCenter(const std::span<const gpu::Vertex> vertices)
    {
        glm::vec3 center = Vector3::zero;

        for (const auto& vertex : vertices)
        {
            center += glm::vec3(vertex.posAndU.x, vertex.posAndU.y, vertex.posAndU.z);
        }

        return center / static_cast<float>(vertices.size());
    }

    static float CalculateRadius(const glm::vec3 center, const std::span<const gpu::Vertex> vertices)
    {
        float radius = 0.0f;

        for (const auto& vertex : vertices)
        {
            radius = std::max(radius, glm::distance(center,
                glm::vec3(vertex.posAndU.x, vertex.posAndU.y, vertex.posAndU.z)));
        }

        return radius;
    }

    static void GeneratePrimitive(const cgltf_primitive& cgltfPrimitive, RawScene& rawScene)
    {
        const auto firstVertexOffset = static_cast<uint32_t>(rawScene.vertices.size());

        auto vertexCount = static_cast<uint32_t>(cgltfPrimitive.attributes[0].data->count);
        const auto indexCount = static_cast<uint32_t>(cgltfPrimitive.indices->count);

        rawScene.vertices.resize(rawScene.vertices.size() + vertexCount);
        auto vertices = std::span(rawScene.vertices).last(vertexCount);

        std::vector<uint32_t> indices;
        indices.resize(indexCount);

        LoadVertices(cgltfPrimitive, vertices);
        LoadIndices(cgltfPrimitive, indices);

        const uint32_t removedVertices = OptimizePrimitive(vertices, indices);

        rawScene.vertices.resize(rawScene.vertices.size() - removedVertices);
        vertexCount -= removedVertices;

        gpu::Primitive& primitive = rawScene.primitives.emplace_back();

        primitive.center = CalculateCenter(vertices);
        primitive.radius = CalculateRadius(primitive.center, vertices);
        primitive.vertexOffset = firstVertexOffset;
        primitive.vertexCount = vertexCount;
        primitive.lodCount = 0;

        // TODO: Load raw attributes to separate arrays and then merge instead of unmerging in cases like this
        std::vector<glm::vec3> positions;
        positions.reserve(vertices.size());

        std::ranges::transform(vertices, std::back_inserter(positions), [](const gpu::Vertex& v)
        {
            return glm::vec3(v.posAndU.x, v.posAndU.y, v.posAndU.z);
        });

        // TODO: Same
        std::vector<glm::vec3> normals;
        normals.reserve(normals.size());

        std::ranges::transform(vertices, std::back_inserter(normals), [](const gpu::Vertex& v)
        {
            return glm::vec3(v.normalAndV.x, v.normalAndV.y, v.normalAndV.z);
        });

        const float lodScale = meshopt_simplifyScale(&positions[0].x, vertices.size(), sizeof(glm::vec3));
        float lodError = 0.0f;

        constexpr std::array normalWeights = { 1.0f, 1.0f, 1.0f };

        for (gpu::Lod& lod : primitive.lods)
        {
            ++primitive.lodCount;

            lod.indexOffset = static_cast<uint32_t>(rawScene.indices.size());
            lod.indexCount = static_cast<uint32_t>(indices.size());
            lod.meshletOffset = 0;
            lod.meshletCount = 0;
            lod.error = lodError * lodScale;

            rawScene.indices.insert(rawScene.indices.end(), indices.begin(), indices.end());

            if (primitive.lodCount < gpu::maxLodCount)
            {
                constexpr float maxError = 0.1f; // 10% of primitive extents
                float nextError = 0.0f;

                const auto nextIndicesTarget = (static_cast<size_t>(static_cast<double>(indices.size()) * 0.65f) / 3) * 3;

                const size_t nextIndices = meshopt_simplifyWithAttributes(indices.data(), indices.data(),
                    indices.size(), &positions[0].x, positions.size(), sizeof(glm::vec3), &normals[0].x,
                    sizeof(glm::vec3), normalWeights.data(), normalWeights.size(), nullptr, nextIndicesTarget, maxError,
                    0, &nextError);

                Assert(nextIndices <= indices.size());

                if (nextIndices == indices.size() || nextIndices == 0)
                {
                    break;
                }

                if (nextIndices >= static_cast<size_t>(static_cast<double>(indices.size()) * 0.95f))
                {
                    break;
                }

                indices.resize(nextIndices);

                meshopt_optimizeVertexCache(indices.data(), indices.data(), indices.size(), positions.size());

                lodError = std::max(lodError, nextError);
            }
        }
    }

    static gpu::Meshlet GenerateMeshlet(const meshopt_Meshlet& meshlet, const std::vector<unsigned int>& vertices,
        const std::vector<unsigned char>& triangles, std::vector<uint32_t>& meshletData,
        const uint32_t firstVertexOffset = 0)
    {
        const auto dataOffset = static_cast<uint32_t>(meshletData.size());

        auto meshletVertices = 
            vertices | std::views::drop(meshlet.vertex_offset) | std::views::take(meshlet.vertex_count);

        const auto& [minVertexIt, maxVertexIt] = std::ranges::minmax_element(meshletVertices);
        const auto [minVertex, maxVertex] = std::tie(*minVertexIt, *maxVertexIt);

        const bool bShortVertexOffsets = maxVertex - minVertex < (1 << 16);

        for (unsigned int i = 0; i < meshlet.vertex_count; ++i)
        {
            const unsigned int vertexOffset = meshletVertices[i] - minVertex;

            if (bShortVertexOffsets && i % 2)
            {
                meshletData.back() |= vertexOffset << 16;
            }
            else
            {
                meshletData.push_back(vertexOffset);
            }
        }

        const auto triangleIndexGroups = reinterpret_cast<const unsigned int*>(&triangles[meshlet.triangle_offset]);
        const unsigned int triangleIndexGroupsCount = (meshlet.triangle_count * 3 + 3) / 4;

        for (unsigned int i = 0; i < triangleIndexGroupsCount; ++i)
        {
            meshletData.push_back(triangleIndexGroups[i]);
        }

        return {
            .dataOffset = dataOffset,
            .firstVertexOffset = firstVertexOffset + minVertex,
            .vertexCount = static_cast<uint8_t>(meshlet.vertex_count),
            .triangleCount = static_cast<uint8_t>(meshlet.triangle_count),
            .bShortVertexOffsets = bShortVertexOffsets, };
    }

    static size_t GenerateMeshlets(const std::span<const gpu::Vertex> vertices, const std::span<const uint32_t> indices, 
        std::vector<gpu::Meshlet>& meshlets, std::vector<uint32_t>& meshletData,
        const uint32_t firstVertexOffset = 0 /* 1st meshlet vertex in global vertex buffer */)
    {
        using namespace SceneHelpersDetails;

        std::vector<meshopt_Meshlet> meshoptMeshlets(indices.size() / 3);
        // These are actually indices to the original vertex array, but that's meshoptimizer naming
        std::vector<unsigned int> meshletVertices(meshoptMeshlets.size() * gpu::maxMeshletVertices);
        std::vector<unsigned char> meshletTriangles(meshoptMeshlets.size() * gpu::maxMeshletTriangles * 3);

        std::vector<glm::vec3> positions;
        positions.reserve(vertices.size());

        // meshopt_buildMeshlets needs positions, not vertices
        std::ranges::transform(vertices, std::back_inserter(positions), [](const gpu::Vertex& v)
        {
            return glm::vec3(v.posAndU.x, v.posAndU.y, v.posAndU.z);
        });

        const size_t meshletCount = meshopt_buildMeshlets(meshoptMeshlets.data(), meshletVertices.data(),
            meshletTriangles.data(), indices.data(), indices.size(), &positions[0].x, positions.size(), 
            sizeof(glm::vec3), gpu::maxMeshletVertices, gpu::maxMeshletTriangles, coneWeight);

        meshoptMeshlets.resize(meshletCount);
        meshletVertices.resize(meshletCount * gpu::maxMeshletVertices);
        meshletTriangles.resize(meshletCount * gpu::maxMeshletTriangles * 3);

        for (const meshopt_Meshlet& meshlet : meshoptMeshlets)
        {
            meshopt_optimizeMeshlet(&meshletVertices[meshlet.vertex_offset], &meshletTriangles[meshlet.triangle_offset], 
                meshlet.triangle_count, meshlet.vertex_count);

            meshlets.push_back(GenerateMeshlet(meshlet, meshletVertices, meshletTriangles, meshletData, 
                firstVertexOffset));
        }

        return meshoptMeshlets.size();
    }

    // Returns mapping from gltf mesh to mesh in our raw scene
    static std::unordered_map<size_t, size_t> ProcessGeometry(const cgltf_data& gltfData, RawScene& rawScene)
    {
        std::unordered_map<size_t, size_t> gltfMeshToMesh;

        // TODO: EXT_mesh_gpu_instancing
        for (size_t i = 0; i < gltfData.meshes_count; ++i) // TODO: counted view?
        {
            const cgltf_mesh& mesh = gltfData.meshes[i];

            uint32_t primitiveCount = 0;

            for (size_t j = 0; j < mesh.primitives_count; ++j)
            {
                const cgltf_primitive& primitive = mesh.primitives[j];

                if (primitive.type != cgltf_primitive_type_triangles || !primitive.indices)
                {
                    continue;
                }

                GeneratePrimitive(primitive, rawScene);

                ++primitiveCount;
            }

            if (primitiveCount != 0)
            {
                gltfMeshToMesh.emplace(i, rawScene.meshes.size());
                rawScene.meshes.emplace_back(static_cast<uint32_t>(rawScene.primitives.size() - primitiveCount),
                    primitiveCount, Matrix4::identity);
            }
        }

        return gltfMeshToMesh;
    }

    static void LoadTransforms(const cgltf_data& gltfData, RawScene& rawScene, 
        const std::unordered_map<size_t, size_t>& gltfMeshToMesh)
    {
        for (size_t i = 0; i < gltfData.nodes_count; ++i)
        {
            const cgltf_node& node = gltfData.nodes[i];

            if (node.mesh && gltfMeshToMesh.contains(cgltf_mesh_index(&gltfData, node.mesh)))
            {
                Mesh& mesh = rawScene.meshes[gltfMeshToMesh.at(cgltf_mesh_index(&gltfData, node.mesh))];

                cgltf_node_transform_world(&node, glm::value_ptr(mesh.transform));
            }
        }
    }

    static std::tuple<glm::vec3, float> CalculateSceneBoundingSphere(const RawScene& rawScene)
    {
        auto sceneMin = glm::vec3(std::numeric_limits<float>::max());
        auto sceneMax = glm::vec3(-std::numeric_limits<float>::max());
        
        for (const Mesh& mesh : rawScene.meshes)
        {
            for (uint32_t index : std::ranges::views::iota(mesh.firstPrimitiveIndex, mesh.firstPrimitiveIndex + mesh.primitiveCount))
            {
                const gpu::Primitive& primitive = rawScene.primitives[index];
                
                const glm::vec3 worldCenter = glm::vec3(mesh.transform * glm::vec4(primitive.center, 1.0f));
                
                const glm::vec3 axisX = glm::vec3(mesh.transform[0]);
                const glm::vec3 axisY = glm::vec3(mesh.transform[1]);
                const glm::vec3 axisZ = glm::vec3(mesh.transform[2]);

                const float maxScale = std::max(std::max(glm::length(axisX), glm::length(axisY)), glm::length(axisZ));
                const float worldRadius = primitive.radius * maxScale;
                
                sceneMin = glm::min(sceneMin, worldCenter - glm::vec3(worldRadius));
                sceneMax = glm::max(sceneMax, worldCenter + glm::vec3(worldRadius));
            }
        }

        const glm::vec3 sceneCenter = (sceneMin + sceneMax) * 0.5f;
        
        return { sceneCenter, glm::length(sceneMax - sceneCenter) };
    }

    static void RandomlyCopyScene(const RawScene& rawScene, std::vector<gpu::Draw>& draws)
    {
        static constexpr size_t maxDrawCount = gpu::primitiveCullMaxCommands;
        
        const size_t singleSceneDrawCount = draws.size();
        const size_t sceneCountMultiplier = maxDrawCount / singleSceneDrawCount;
        
        const auto& [sceneCenter, sceneRadius] = SceneHelpersDetails::CalculateSceneBoundingSphere(rawScene);

        const float cubeHalfSize = std::cbrt(static_cast<float>(sceneCountMultiplier)) * (sceneRadius * 2.0f) * 0.5f;

        std::mt19937 rng(228);

        std::uniform_real_distribution<float> positionDist(-cubeHalfSize, cubeHalfSize);
        std::uniform_real_distribution<float> angleDist(0.0f, glm::two_pi<float>());
        std::uniform_real_distribution<float> scaleDist(0.2f, 1.0f);
        
        std::vector<glm::vec3> scenePositions;
        scenePositions.reserve(sceneCountMultiplier);
        
        // Preproceess positions to sort them
        for (size_t i = 0; i < sceneCountMultiplier - 1; ++i)
        {
            scenePositions.push_back(glm::vec3(positionDist(rng), positionDist(rng), positionDist(rng)));
        }
        
        std::ranges::sort(scenePositions, {}, [](const glm::vec3& v) { return glm::length2(v); });

        // Copy the whole scene with random transforms enough times to reach required number of issued draws
        for (size_t i = 0; i < sceneCountMultiplier - 1; ++i)
        {
            const float scaleFactor = scaleDist(rng);
            const auto sceneRotationQuat = glm::quat(glm::vec3(angleDist(rng), angleDist(rng), angleDist(rng)));

            for (size_t j = 0; j < singleSceneDrawCount; ++j)
            {
                if (draws.size() == gpu::primitiveCullMaxCommands)
                {
                    return;
                }
                
                const auto& base = draws[j];

                const glm::vec3 localPos = base.position - sceneCenter;
                const glm::vec3 transformedPos = sceneRotationQuat * (localPos * scaleFactor);
                const glm::vec3 finalPos = transformedPos + scenePositions[i];
                const float finalScale = base.scale * scaleFactor;

                const auto baseRotationQuat = glm::quat(base.rotation.w, base.rotation.x, base.rotation.y, base.rotation.z);
                const glm::quat finalRotationQuat = sceneRotationQuat * baseRotationQuat;
                const auto finalRotation = glm::vec4(finalRotationQuat.x, finalRotationQuat.y, finalRotationQuat.z, finalRotationQuat.w);

                draws.emplace_back(finalPos, finalScale, finalRotation, base.primitiveIndex);
            }
        }
    }
}

std::optional<RawScene> SceneHelpers::LoadGltfScene(const FilePath& path)
{
    using namespace SceneHelpersDetails;

    RawScene rawScene;

    const std::string absolutePath = path.GetAbsolute();

    cgltf_options options = {};
    cgltf_data* gltfData = nullptr;

    {
        ScopeTimer timer("Parse gltf scene");

        if (cgltf_parse_file(&options, absolutePath.data(), &gltfData) != cgltf_result_success)
        {
            return std::nullopt;
        }
    }

    std::unique_ptr<cgltf_data, void (*)(cgltf_data*)> gltfDataPtr(gltfData, &cgltf_free);

    {
        ScopeTimer timer("Load gltf scene buffers & validate");

        if (cgltf_load_buffers(&options, gltfData, absolutePath.data()) != cgltf_result_success)
        {
            return std::nullopt;
        }

        if (cgltf_validate(gltfData) != cgltf_result_success)
        {
            return std::nullopt;
        }
    }

    {
        ScopeTimer timer("Process gltf scene");

        std::unordered_map<size_t, size_t> gltfMeshToMesh = ProcessGeometry(*gltfData, rawScene);

        LoadTransforms(*gltfData, rawScene, gltfMeshToMesh);
    }

    return rawScene;
}

void SceneHelpers::GenerateMeshlets(RawScene& rawScene)
{
    ScopeTimer timer("Generate meshlets");

    for (gpu::Primitive& primitive : rawScene.primitives)
    {
        const auto vertices = std::span(rawScene.vertices.data() + primitive.vertexOffset, primitive.vertexCount);

        for (uint32_t i = 0; i < primitive.lodCount; ++i)
        {
            gpu::Lod& lod = primitive.lods[i];

            const auto indices = std::span(rawScene.indices.data() + lod.indexOffset, lod.indexCount);

            lod.meshletOffset = static_cast<uint32_t>(rawScene.meshlets.size());
            lod.meshletCount = static_cast<uint32_t>(SceneHelpersDetails::GenerateMeshlets(vertices, indices,
                rawScene.meshlets, rawScene.meshletData, primitive.vertexOffset));
        }
    }
}

std::vector<gpu::Draw> SceneHelpers::GenerateDraws(const RawScene& rawScene)
{
    std::vector<gpu::Draw> draws;
    
    for (const Mesh& mesh : rawScene.meshes)
    {
        glm::vec3 position;
        glm::vec3 scale;
        glm::quat rotationQuat;
        glm::vec3 skew;
        glm::vec4 perspective;
        
        glm::decompose(mesh.transform, scale, rotationQuat, position, skew, perspective);
        
        const float scaleScalar = (scale.x + scale.y + scale.z) / 3.0f;
        const glm::vec4 rotation = glm::vec4(rotationQuat.x, rotationQuat.y, rotationQuat.z, rotationQuat.w);
        
        for (uint32_t index : std::ranges::views::iota(mesh.firstPrimitiveIndex, mesh.firstPrimitiveIndex + mesh.primitiveCount))
        {
            if (draws.size() == gpu::primitiveCullMaxCommands)
            {
                return draws;
            }
            
            draws.emplace_back(position, scaleScalar, rotation, index);
        }
    }
    
    if constexpr (EngineConfig::randomlyCopyScene)
    {
        SceneHelpersDetails::RandomlyCopyScene(rawScene, draws);
    }

    return draws;
}

std::vector<VkVertexInputBindingDescription> SceneHelpers::GetVertexBindings()
{
    VkVertexInputBindingDescription binding{};
    binding.binding = 0;
    binding.stride = sizeof(gpu::Vertex);
    binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    return { binding };
}

// TODO: (low priority) parse from compiled shader file
std::vector<VkVertexInputAttributeDescription> SceneHelpers::GetVertexAttributes()
{
    std::vector<VkVertexInputAttributeDescription> attributes(4);

    VkVertexInputAttributeDescription& posAttribute = attributes[0];
    posAttribute.binding = 0;
    posAttribute.location = 0;
    posAttribute.format = VK_FORMAT_R32G32B32A32_SFLOAT;
    posAttribute.offset = offsetof(gpu::Vertex, posAndU);

    VkVertexInputAttributeDescription& normalAttribute = attributes[1];
    normalAttribute.binding = 0;
    normalAttribute.location = 1;
    normalAttribute.format = VK_FORMAT_R32G32B32A32_SFLOAT;
    normalAttribute.offset = offsetof(gpu::Vertex, normalAndV);

    VkVertexInputAttributeDescription& tangentAttribute = attributes[2];
    tangentAttribute.binding = 0;
    tangentAttribute.location = 2;
    tangentAttribute.format = VK_FORMAT_R32G32B32A32_SFLOAT;
    tangentAttribute.offset = offsetof(gpu::Vertex, tangent);

    VkVertexInputAttributeDescription& colorAttribute = attributes[3];
    colorAttribute.binding = 0;
    colorAttribute.location = 3;
    colorAttribute.format = VK_FORMAT_R32G32B32_SFLOAT;
    colorAttribute.offset = offsetof(gpu::Vertex, color);

    return attributes;
}
