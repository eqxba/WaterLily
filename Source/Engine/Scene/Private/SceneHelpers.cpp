#include "Engine/Scene/SceneHelpers.hpp"

#include "Utils/Helpers.hpp"

DISABLE_WARNINGS_BEGIN

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define TINYGLTF_USE_CPP14
#include <tiny_gltf.h>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

DISABLE_WARNINGS_END

namespace SceneHelpersDetails
{
    static glm::mat4 GetTransform(const tinygltf::Node& node)
    {
		glm::mat4 transform = Matrix4::identity;

		if (node.matrix.size() == 16)
		{
			transform = glm::make_mat4x4(node.matrix.data());
		}
		else
		{
			if (node.translation.size() == 3)
			{
				transform = glm::translate(transform, glm::vec3(glm::make_vec3(node.translation.data())));
			}

			if (node.rotation.size() == 4)
			{
				const glm::quat q = glm::make_quat(node.rotation.data());
				transform *= glm::mat4(q);
			}

			if (node.scale.size() == 3)
			{
				transform = glm::scale(transform, glm::vec3(glm::make_vec3(node.scale.data())));
			}
		}

		return transform;
    }

	static const unsigned char* GetAttributeData(const tinygltf::Model& model, const tinygltf::Accessor& accessor)
	{
		const tinygltf::BufferView& view = model.bufferViews[accessor.bufferView];
		const std::vector<unsigned char>& data = model.buffers[view.buffer].data;
		const size_t offset = accessor.byteOffset + view.byteOffset;

		const unsigned char* bufferPtr = &data[offset];

		return bufferPtr;
	}

	static std::span<const float> GetAttributeData(const tinygltf::Model& model, const tinygltf::Primitive& primitive, 
		const char* attributeName, const size_t floatsPerAttribute)
    {
		std::span<const float> result;

		if (const auto attribute = primitive.attributes.find(attributeName); attribute != primitive.attributes.end())
		{
			const tinygltf::Accessor& accessor = model.accessors[attribute->second];
			const unsigned char* bufferPtr = GetAttributeData(model, accessor);
			result = std::span(reinterpret_cast<const float*>(bufferPtr), accessor.count * floatsPerAttribute);
		}

		return result;
    }

	static std::vector<uint32_t> GetIndices(const int componentType, const unsigned char* bufferPtr, 
		const size_t count, const uint32_t customIndexOffset)
    {
		std::vector<uint32_t> result;

		switch (componentType)
		{
		case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT:
		{
			const auto* buf32 = reinterpret_cast<const uint32_t*>(bufferPtr);
			for (size_t index = 0; index < count; ++index)
			{
				result.push_back(buf32[index] + customIndexOffset);
			}
			break;
		}
		case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT:
		{
			const auto* buf16 = reinterpret_cast<const uint16_t*>(bufferPtr);
			for (size_t index = 0; index < count; index++)
			{
				result.push_back(buf16[index] + customIndexOffset);
			}
			break;
		}
		case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE:
		{
			const auto* buf8 = reinterpret_cast<const uint8_t*>(bufferPtr);
			for (size_t index = 0; index < count; index++) 
			{
				result.push_back(buf8[index] + customIndexOffset);
			}
			break;
		}
		default:
			Assert(false);
		}

		return result;
    }

	static void ProcessPrimitiveVertices(const tinygltf::Model& model, const tinygltf::Primitive& primitive, 
		std::vector<Vertex>& vertices)
    {
		std::span<const float> positions = GetAttributeData(model, primitive, "POSITION", 3);
		std::span<const float> normals = GetAttributeData(model, primitive, "NORMAL", 3);
		std::span<const float> uvs = GetAttributeData(model, primitive, "TEXCOORD_0", 2);
		std::span<const float> tangents = GetAttributeData(model, primitive, "TANGENT", 4);

		Assert(!positions.empty());

		const size_t vertexCount = positions.size() / 3;

		for (size_t index = 0; index < vertexCount; ++index)
		{
            vertices.push_back({
				glm::vec4(glm::make_vec3(&positions[index * 3]), 1.0f),
				normals.empty() ? Vector3::zero : glm::make_vec3(&normals[index * 3]),
				uvs.empty() ? Vector2::zero : glm::make_vec2(&uvs[index * 2]),
                tangents.empty() ? Vector4::zero : glm::make_vec4(&tangents[index * 4])});
		}
    }

	static uint32_t ProcessPrimitiveIndices(const tinygltf::Model& model, const tinygltf::Primitive& primitive,
		std::vector<uint32_t>& indices, const uint32_t firstVertex)
    {
		const tinygltf::Accessor& accessor = model.accessors[primitive.indices];
		const unsigned char* bufferPtr = GetAttributeData(model, accessor);

		indices.insert_range(indices.end(), GetIndices(accessor.componentType, bufferPtr, accessor.count, firstVertex));

		return static_cast<uint32_t>(accessor.count);
    }
}

std::tuple<std::vector<Vertex>, std::vector<uint32_t>> SceneHelpers::LoadObjModel(const std::string& absolutePath,
    const bool flipYz /* = false */)
{
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    tinyobj::attrib_t attributes;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;

    std::string warn, err;

    if (!tinyobj::LoadObj(&attributes, &shapes, &materials, &warn, &err, absolutePath.c_str()))
    {
        Assert(false);
    }

    std::unordered_map<Vertex, uint32_t> uniqueVertices{};

    const auto processMeshIndex = [&](const tinyobj::index_t& index) {
        Vertex vertex{};

        vertex.pos = {
            attributes.vertices[3 * index.vertex_index + 0],
            attributes.vertices[3 * index.vertex_index + 1],
            attributes.vertices[3 * index.vertex_index + 2], };

        if (flipYz)
        {
            std::swap(vertex.pos.y, vertex.pos.z);
            vertex.pos.z *= -1;
        }

		vertex.normal = Vector3::zero;

        vertex.uv = {
            attributes.texcoords[2 * index.texcoord_index + 0],
            1.0f - attributes.texcoords[2 * index.texcoord_index + 1], };

		vertex.color = Vector3::allOnes;
		vertex.tangent = Vector4::allOnes;

        if (uniqueVertices.count(vertex) == 0)
        {
            uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
            vertices.push_back(vertex);
        }

        indices.push_back(uniqueVertices[vertex]);
    };

    std::ranges::for_each(shapes, [&](const auto& shape) {
        std::ranges::for_each(shape.mesh.indices, processMeshIndex);
    });

    return { std::move(vertices), std::move(indices) };
}

std::unique_ptr<tinygltf::Model> SceneHelpers::LoadGltfScene(const std::string& absolutePath)
{
	ScopeTimer timer("Load gltf scene");

    auto model = std::make_unique<tinygltf::Model>();
    tinygltf::TinyGLTF loader;
    std::string err;
    std::string warn;

    const bool success = loader.LoadASCIIFromFile(model.get(), &err, &warn, absolutePath);
    Assert(success);

    return model;
}

std::unique_ptr<SceneNode> SceneHelpers::LoadGltfHierarchy(const tinygltf::Node& gltfNode, const tinygltf::Model& model,
	std::vector<uint32_t>& indices, std::vector<Vertex>& vertices)
{
	using namespace SceneHelpersDetails;

	auto node = std::make_unique<SceneNode>();

	node->name = gltfNode.name;
	node->transform = GetTransform(gltfNode);

	if (!gltfNode.children.empty())
	{
		const auto loadChild = [&](const int childIndex) {
			std::unique_ptr<SceneNode> child = LoadGltfHierarchy(model.nodes[childIndex], model, indices, vertices);
			child->parent = node.get();

			return child;
		};

		std::ranges::transform(gltfNode.children, std::back_inserter(node->children), loadChild);
	}

	if (gltfNode.mesh > -1) 
	{
		const auto producePrimitive = [&](const auto& gltfPrimitive) {
			const auto firstIndex = static_cast<uint32_t>(indices.size());
			const auto firstVertex = static_cast<uint32_t>(vertices.size());

			const uint32_t indexCount = ProcessPrimitiveIndices(model, gltfPrimitive, indices, firstVertex);
			ProcessPrimitiveVertices(model, gltfPrimitive, vertices);

			Primitive primitive{};
			primitive.firstIndex = firstIndex;
			primitive.indexCount = indexCount;

			return primitive;
		};

		const tinygltf::Mesh mesh = model.meshes[gltfNode.mesh];

		std::ranges::transform(mesh.primitives, std::back_inserter(node->mesh.primitives), producePrimitive);
	}

	return node;
}