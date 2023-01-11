#pragma once
#include "../core/icpMacros.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <vulkan/vulkan.hpp>
#include <array>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>


INCEPTION_BEGIN_NAMESPACE

class icpImageResource;
enum class ePrimitiveType;

struct icpVertex
{
	glm::vec3 position;
	glm::vec3 color;
	glm::vec2 texCoord;

	static VkVertexInputBindingDescription getBindingDescription()
	{
		VkVertexInputBindingDescription bindingDescription{};

		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(icpVertex);
		bindingDescription.inputRate = VkVertexInputRate::VK_VERTEX_INPUT_RATE_VERTEX;

		return bindingDescription;
	}

	static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescription()
	{
		std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};
		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(icpVertex, position);

		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(icpVertex, color);

		attributeDescriptions[2].binding = 0;
		attributeDescriptions[2].location = 2;
		attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[2].offset = offsetof(icpVertex, texCoord);

		return attributeDescriptions;

	}

	bool operator==(const icpVertex& v1) const
	{
		return position == v1.position
			&& color == v1.color
			&& texCoord == v1.texCoord;
	}
};


class icpMeshData
{
public:
	std::vector<icpVertex> m_vertices;
	std::vector<uint32_t> m_vertexIndices;

	std::vector<VkDescriptorSet> m_descriptorSets;

	std::vector<VkBuffer> m_uniformBuffers;
	std::vector<VkDeviceMemory> m_uniformBufferMem;

	VkBuffer m_vertexBuffer;
	VkBuffer m_indexBuffer;
	VkDeviceMemory m_vertexBufferMem;
	VkDeviceMemory m_indexBufferMem;

	VkImage m_textureImage;
	VkDeviceMemory m_textureBufferMem;
	VkImageView m_textureImageView;
	VkSampler m_textureSampler;

	std::string m_meshName;

	std::shared_ptr<icpImageResource> m_imgRes = nullptr;

	void createVertexBuffers();
	void createIndexBuffers();
	void createUniformBuffers();


	void allocateDescriptorSets();

	void createTextureImages();
	void createTextureImageViews(size_t mipmaplevel);
	void createTextureSampler();

	void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipmapLevel);
	void generateMipmaps(VkImage image, VkFormat imageFormat, int32_t width, int32_t height, uint32_t mipmapLevels);
	void copyBuffer2Image(VkBuffer srcBuffer, VkImage dstImage, uint32_t width, uint32_t height);

	void fillInPrimitiveData(ePrimitiveType type);
	void fillInCubeData();

	void createCubeVertexBuffers();
	void createCubeIndexBuffers();
};

INCEPTION_END_NAMESPACE

namespace std
{
	template<>
	struct hash<Inception::icpVertex>
	{
		size_t operator()(Inception::icpVertex const& vertex) const
		{
			return ((hash<glm::vec3>()(vertex.position) ^ (hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^ (hash<glm::vec2>()(vertex.texCoord) << 1);
		}
	};
}