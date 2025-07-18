#include "icpGLTFLoaderUtil.h"

#include "icpSamplerResource.h"
#include "../core/icpSystemContainer.h"
#include "../core/icpLogSystem.h"
#include "../mesh/icpMeshData.h"
#include "../mesh/icpMeshResource.h"
#include "../render/icpRenderSystem.h"
#include "../render/material/icpMaterial.h"
#include <glm/gtc/type_ptr.hpp>

#include "../scene/icpEntityDataComponent.h"
#include "../scene/icpXFormComponent.h"

INCEPTION_BEGIN_NAMESPACE

void icpGLTFLoaderUtil::LoadGLTFMeshs(tinygltf::Model& gltfModel, std::vector<std::shared_ptr<icpMaterialInstance>>& materials, 
	                                      std::vector<std::vector<icpMeshResource>>& meshResources)
{
	meshResources.resize(gltfModel.meshes.size());
	for (auto meshIdx = 0; meshIdx < gltfModel.meshes.size(); meshIdx++)
	{
		tinygltf::Mesh& gltfMesh = gltfModel.meshes[meshIdx];

		std::vector<icpVertex> vertices;
		std::vector<uint32_t> indices;

		std::vector<icpMeshResource> primitives(gltfMesh.primitives.size());

		for (auto primitiveIdx = 0; primitiveIdx < gltfMesh.primitives.size(); primitiveIdx++)
		{
			tinygltf::Primitive& gltfPrimitive = gltfMesh.primitives[primitiveIdx];

			icpMeshData meshData;
			meshData.m_eVertexFormat = eVertexFormat::PNCV_F32;

			LoadGLTFIndexBuffer(gltfPrimitive, gltfModel, meshData);
			LoadGLTFVertexBuffer(gltfPrimitive, gltfModel, meshData);

			icpMeshResource meshRes;
			meshRes.m_meshData = meshData;
			meshRes.m_pMaterial = materials[gltfPrimitive.material];
			meshRes.m_resType = icpResourceType::MESH;
			meshRes.m_id = (gltfMesh.name.empty() ? "AnonymousMesh" : gltfMesh.name) + "_Primitive" + std::to_string(primitiveIdx);
			g_system_container.m_resourceSystem->LoadModelResource(meshRes);

			primitives[primitiveIdx] = meshRes;
		}

		meshResources[meshIdx] = primitives;
	}
}

void icpGLTFLoaderUtil::LoadGLTFBuffer(
	uint8_t* dataPtr, const tinygltf::Accessor& accessor, const tinygltf::BufferView& bufferView,
	std::vector<uint8_t>& outBuffer)
{
	size_t elementSize = tinygltf::GetComponentSizeInBytes(accessor.componentType);
	size_t elementCount = tinygltf::GetNumComponentsInType(accessor.type);

	elementSize *= elementCount;

	size_t stride = bufferView.byteStride;
	if (stride == 0)
	{
		stride = elementSize;

	}

	size_t indexCount = accessor.count;

	outBuffer.resize(indexCount * elementSize);

	for (int i = 0; i < indexCount; i++) 
	{
		uint8_t* dataindex = dataPtr + stride * i;
		uint8_t* targetptr = outBuffer.data() + elementSize * i;

		memcpy(targetptr, dataindex, elementSize);
	}
}

void icpGLTFLoaderUtil::LoadGLTFIndexBuffer(
	const tinygltf::Primitive& gltfPrimitive, const tinygltf::Model& gltfModel,
	icpMeshData& meshData)
{
	// Load Index Buffer
	int accessorIdx = gltfPrimitive.indices;

	const tinygltf::Accessor& accessor = gltfModel.accessors[accessorIdx > -1 ? accessorIdx : 0];
	const tinygltf::BufferView& bufferView = gltfModel.bufferViews[accessor.bufferView];
	const tinygltf::Buffer& buffer = gltfModel.buffers[bufferView.buffer];

	uint32_t indexCount = static_cast<uint32_t>(accessor.count);
	uint8_t* dataPtr = (uint8_t*)buffer.data.data() + accessor.byteOffset + bufferView.byteOffset;

	std::vector<uint8_t> indexBufferData;
	LoadGLTFBuffer(dataPtr, accessor, bufferView, indexBufferData);
	for (int i = 0; i < indexCount; i++)
	{
		uint32_t index = 0;
		switch (accessor.componentType)
		{
		case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
		{
			uint32_t* bfr = (uint32_t*)indexBufferData.data();
			index = *(bfr + i);
		}
		break;
		case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
		{
			uint16_t* bfr = (uint16_t*)indexBufferData.data();
			index = *(bfr + i);
		}
		break;
		case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
		{
			uint8_t* bfr = (uint8_t*)indexBufferData.data();
			index = *(bfr + i);
		}
		break;
		default:
		{
			ICP_LOG_WARING("Unsupported index format");
		}
		}
		meshData.m_vertexIndices.push_back(index);
	}
}

void icpGLTFLoaderUtil::LoadGLTFVertexBuffer(
	const tinygltf::Primitive& gltfPrimitive, const tinygltf::Model& gltfModel,
	icpMeshData& meshData)
{
	// Load Vertex Data
	assert(gltfPrimitive.attributes.contains("POSITION"));

	const int posAccessorIdx = gltfPrimitive.attributes.at("POSITION");

	const tinygltf::Accessor& posAccessor = gltfModel.accessors[posAccessorIdx > -1 ? posAccessorIdx : 0];
	assert(posAccessor.type == TINYGLTF_TYPE_VEC3);
	assert(posAccessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);

	const tinygltf::BufferView& posBufferView = gltfModel.bufferViews[posAccessor.bufferView];
	const tinygltf::Buffer& posBuffer = gltfModel.buffers[posBufferView.buffer];

	auto vertexCount = static_cast<uint32_t>(posAccessor.count);
	uint8_t* posDataPtr = (uint8_t*)&posBuffer.data[posAccessor.byteOffset + posBufferView.byteOffset];

	std::vector<uint8_t> posBufferData;
	LoadGLTFBuffer(posDataPtr, posAccessor, posBufferView, posBufferData);

	std::vector<uint8_t> normalBufferData;
	int normalAccessorIdx = -1;
	if (gltfPrimitive.attributes.contains("NORMAL"))
	{
		normalAccessorIdx = gltfPrimitive.attributes.at("NORMAL");
		const tinygltf::Accessor& accessor = gltfModel.accessors[normalAccessorIdx > -1 ? normalAccessorIdx : 0];
		assert(accessor.type == TINYGLTF_TYPE_VEC3);
		assert(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);

		const tinygltf::BufferView& bufferView = gltfModel.bufferViews[accessor.bufferView];
		const tinygltf::Buffer& buffer = gltfModel.buffers[bufferView.buffer];

		uint8_t* dataPtr = (uint8_t*)&buffer.data[accessor.byteOffset + bufferView.byteOffset];

		LoadGLTFBuffer(dataPtr, accessor, bufferView, normalBufferData);
	}

	std::vector<uint8_t> colorBufferData;
	int colorAccessorIdx = -1;
	if (gltfPrimitive.attributes.contains("COLOR_0"))
	{
		colorAccessorIdx = gltfPrimitive.attributes.at("COLOR_0");
		const tinygltf::Accessor& accessor = gltfModel.accessors[colorAccessorIdx > -1 ? colorAccessorIdx : 0];
		assert(accessor.type == TINYGLTF_TYPE_VEC3);
		assert(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);

		const tinygltf::BufferView& bufferView = gltfModel.bufferViews[accessor.bufferView];
		const tinygltf::Buffer& buffer = gltfModel.buffers[bufferView.buffer];

		uint8_t* dataPtr = (uint8_t*)&(buffer.data[accessor.byteOffset + bufferView.byteOffset]);

		LoadGLTFBuffer(dataPtr, accessor, bufferView, colorBufferData);
	}

	std::vector<uint8_t> texCoordBufferData;
	int tcAccessorIdx = -1;
	if (gltfPrimitive.attributes.contains("TEXCOORD_0"))
	{
		tcAccessorIdx = gltfPrimitive.attributes.at("TEXCOORD_0");
		const tinygltf::Accessor& accessor = gltfModel.accessors[tcAccessorIdx > -1 ? tcAccessorIdx : 0];
		assert(accessor.type == TINYGLTF_TYPE_VEC2);
		assert(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);

		const tinygltf::BufferView& bufferView = gltfModel.bufferViews[accessor.bufferView];
		const tinygltf::Buffer& buffer = gltfModel.buffers[bufferView.buffer];

		uint8_t* dataPtr = (uint8_t*)&(buffer.data[accessor.byteOffset + bufferView.byteOffset]);

		LoadGLTFBuffer(dataPtr, accessor, bufferView, texCoordBufferData);
	}

	for (size_t i = 0; i < vertexCount; i++)
	{
		icpVertex vertex;

		float* pPos = (float*)posBufferData.data();

		vertex.position[0] = *(pPos + (i * 3) + 0);
		vertex.position[1] = *(pPos + (i * 3) + 1);
		vertex.position[2] = *(pPos + (i * 3) + 2);

		if (!normalBufferData.empty())
		{
			float* pNormal = (float*)normalBufferData.data();

			vertex.normal[0] = *(pNormal + (i * 3) + 0);
			vertex.normal[1] = *(pNormal + (i * 3) + 1);
			vertex.normal[2] = *(pNormal + (i * 3) + 2);
		}

		if (!texCoordBufferData.empty())
		{
			float* pTc = (float*)texCoordBufferData.data();

			vertex.texCoord[0] = *(pTc + (i * 2) + 0);
			vertex.texCoord[1] = *(pTc + (i * 2) + 1);
		}

		if (!colorBufferData.empty())
		{
			float* pColor = (float*)colorBufferData.data();

			vertex.color[0] = *(pColor + (i * 3) + 0);
			vertex.color[1] = *(pColor + (i * 3) + 1);
			vertex.color[2] = *(pColor + (i * 3) + 2);
		}

		meshData.m_vertices.push_back(vertex);
	}
}

VkSamplerAddressMode getVkWrapMode(int32_t wrapMode)
{
	switch (wrapMode)
	{
	case -1:
	case 10497:
		return VK_SAMPLER_ADDRESS_MODE_REPEAT;
	case 33071:
		return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	case 33648:
		return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
	}

	ICP_LOG_ERROR("Unknown wrap mode for getVkWrapMode: ", wrapMode);
	return VK_SAMPLER_ADDRESS_MODE_REPEAT;
}

VkFilter getVkFilterMode(int32_t filterMode)
{
	switch (filterMode)
	{
	case -1:
	case 9728:
		return VK_FILTER_NEAREST;
	case 9729:
		return VK_FILTER_LINEAR;
	case 9984:
		return VK_FILTER_NEAREST;
	case 9985:
		return VK_FILTER_NEAREST;
	case 9986:
		return VK_FILTER_LINEAR;
	case 9987:
		return VK_FILTER_LINEAR;
	}

	ICP_LOG_ERROR("Unknown filter mode for getVkFilterMode: ", filterMode);
	return VK_FILTER_NEAREST;
}

void icpGLTFLoaderUtil::LoadGLTFTextureSamplers(tinygltf::Model& gltfModel, std::vector<icpSamplerResource>& samplers)
{
	for (auto& sampler : gltfModel.samplers)
	{
		icpSamplerResource textureSampler;

		textureSampler.minFilter = getVkFilterMode(sampler.minFilter);
		textureSampler.magFilter = getVkFilterMode(sampler.magFilter);
		textureSampler.addressModeU = getVkWrapMode(sampler.wrapS);
		textureSampler.addressModeV = getVkWrapMode(sampler.wrapT);
		textureSampler.addressModeW = textureSampler.addressModeV;

		samplers.push_back(textureSampler);
	}
}

void icpGLTFLoaderUtil::LoadGLTFTextures(tinygltf::Model& gltfModel, const std::vector<icpSamplerResource>& samplers,
	std::vector<std::string>& imageIDs, const std::filesystem::path& FolderPath)
{
	for (tinygltf::Texture& tex : gltfModel.textures) 
	{
		tinygltf::Image& image = gltfModel.images[tex.source];
		icpSamplerResource textureSampler;
		if (tex.sampler == -1) 
		{
			// No sampler specified, use a default one
			textureSampler.magFilter = VK_FILTER_LINEAR;
			textureSampler.minFilter = VK_FILTER_LINEAR;
			textureSampler.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			textureSampler.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			textureSampler.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		}
		else 
		{
			textureSampler = samplers[tex.sampler];
		}

		std::shared_ptr<icpResourceBase> res = nullptr;
		if (!image.uri.empty())
		{
			auto full_path = FolderPath / image.uri;
			res = g_system_container.m_resourceSystem->loadImageResource(full_path, textureSampler);
		}
		else
		{
			icpImageResource imageRes;
			imageRes.setImageBuffer(image.image.data(), image.image.size(), image.width, image.height, image.component);
			imageRes.m_sampler = textureSampler;
			imageRes.m_mipmapLevel = static_cast<uint32_t>(std::floor(std::log2(std::max(image.width, image.height))));
			imageRes.m_id = "Anonymous_" + std::to_string(tex.source);
			imageRes.m_resType = icpResourceType::TEXTURE;
			res = g_system_container.m_resourceSystem->LoadImageResource(imageRes);
		}

		imageIDs.push_back(res->m_id);
	}
}

void icpGLTFLoaderUtil::LoadGLTFMaterials(tinygltf::Model& gltfModel, const std::vector<std::string>& images, 
	std::vector<std::shared_ptr<icpMaterialInstance>>& materials)
{
	auto materialSystem = g_system_container.m_renderSystem->GetMaterialSubSystem();
	for (auto& material : gltfModel.materials)
	{
		if (material.alphaMode == "BLEND")
		{
			// todo: handle transparent objects
			materials.push_back(nullptr);
			continue;
		}

		auto& pbr = material.pbrMetallicRoughness;

		pbr.baseColorTexture.index = pbr.baseColorTexture.index < 0 ? 0 : pbr.baseColorTexture.index;
		auto instance = materialSystem->createMaterialInstance(eMaterialShadingModel::PBR_LIT);

		instance->m_blendMode = material.alphaMode == "OPAQUE" ? eMaterialBlendMode::OPAQUE : eMaterialBlendMode::MASK;
		instance->AddScalaValue("alphaMask", { static_cast<float>(material.alphaMode == "MASK") });
		instance->AddScalaValue("alphaMaskCutoff", { static_cast<float>(material.alphaCutoff) });

		auto& baseImage = images[pbr.baseColorTexture.index];
		instance->AddTexture("baseColorTexture", { baseImage });

		glm::vec4 baseColorFactor = glm::make_vec4(pbr.baseColorFactor.data());
		instance->AddVector4Value("baseColorFactor", {baseColorFactor});

		if (pbr.metallicRoughnessTexture.index >= 0)
		{
			auto& image = images[pbr.metallicRoughnessTexture.index];
			instance->AddTexture( "metallicRoughnessTexture", { image });
		}

		float metallicFactor = static_cast<float>(pbr.metallicFactor);
		instance->AddScalaValue("metallicFactor", { metallicFactor });

		float roughnessFactor = static_cast<float>(pbr.roughnessFactor);
		instance->AddScalaValue( "roughnessFactor", { roughnessFactor });

		if (material.normalTexture.index >= 0)
		{
			auto& image = images[material.normalTexture.index];
			instance->AddTexture( "normalTexture", { image });
		}

		if (material.occlusionTexture.index >= 0)
		{
			auto& image = images[material.occlusionTexture.index];
			instance->AddTexture( "occlusionTexture", { image });
		}

		if (material.emissiveTexture.index >= 0)
		{
			auto& image = images[material.emissiveTexture.index];
			instance->AddTexture( "emissiveTexture", { image });
		}

		glm::vec4 emissiveFactor = glm::make_vec4(material.emissiveFactor.data());
		instance->AddVector4Value( "emissiveFactor", { emissiveFactor });

		materials.push_back(std::static_pointer_cast<icpMaterialInstance>(instance));
	}
}

void icpGLTFLoaderUtil::LoadGLTFScene(tinygltf::Model& gltfModel, std::vector<std::vector<icpMeshResource>>& meshResources)
{
	const auto& scene = gltfModel.scenes[gltfModel.defaultScene < 0 ? 0 : gltfModel.defaultScene];

	for (int nodeIdx = 0; nodeIdx < scene.nodes.size(); nodeIdx++)
	{
		LoadGLTFNode(gltfModel, scene.nodes[nodeIdx], icpGuid(0u), meshResources, true);
	}
}

void icpGLTFLoaderUtil::LoadGLTFNode(tinygltf::Model& gltfModel, int nodeIdx, icpGuid parentGuid, std::vector<std::vector<icpMeshResource>>& meshResources, bool is_root)
{
	icpGameEntity* parentEntity = nullptr;
	if (!is_root)
	{
		const auto view = g_system_container.m_sceneSystem->m_registry.view<icpEntityDataComponent>();
		const auto it = std::find_if(view.begin(), view.end(), [&view, &parentGuid](auto args)
			{
				icpEntityDataComponent& dataComp = view.get<icpEntityDataComponent>(args);
				return dataComp.m_guid == parentGuid;
			});

		auto& parentDataComp = view.get<icpEntityDataComponent>(*it);
		parentEntity = parentDataComp.m_possessor;
	}

	auto& node = gltfModel.nodes[nodeIdx];
	glm::mat4 worldMtx{1.f};

	if (!node.matrix.empty())
	{
		std::array<float, 16> matrixData;
		for (int n = 0; n < 16; n++)
		{
			matrixData[n] = static_cast<float>(node.matrix[n]);
		}
		memcpy(&worldMtx, matrixData.data(), sizeof(glm::mat4));
	}
	else
	{
		glm::vec3 translation{0.f};
		glm::mat4 translationMtx{ 1.f };
		if (!node.translation.empty())
		{
			translation = glm::vec3{ node.translation[0], node.translation[1], node.translation[2] };
			translationMtx = glm::translate(translationMtx, translation);
		}

		glm::mat4 rotation{ 1.f };

		if (!node.rotation.empty())
		{
			glm::quat rot(node.rotation[3], node.rotation[0], node.rotation[1], node.rotation[2]);
			rotation = glm::mat4{ rot };
		}

		glm::mat4 scale{ 1.f };
		if (!node.scale.empty())
		{
			scale = glm::scale(scale, glm::vec3{ node.scale[0], node.scale[1], node.scale[2] });
		}

		glm::mat4 transformMatrix = (translationMtx * rotation * scale);
		memcpy(&worldMtx, &transformMatrix, sizeof(glm::mat4));
	}

	icpGuid thisGuid = icpGuid(0u);

	if (node.mesh > -1)
	{
		auto& primitives = meshResources[node.mesh];

		for (auto& primitive : primitives)
		{
			if (!primitive.m_pMaterial)
			{
				continue;
			}

			auto entity = g_system_container.m_sceneSystem->CreateEntity(primitive.m_id, nullptr);
			auto& xform = entity->accessComponent<icpXFormComponent>();

			xform.m_mtxTransform = worldMtx;

			if (parentEntity)
			{
				auto parentXform = std::make_shared<icpXFormComponent>(parentEntity->accessComponent<icpXFormComponent>());
				xform.m_parent = parentXform;
				parentXform->m_children.push_back(std::make_shared<icpXFormComponent>(xform));
			}

			auto& meshComp = entity->installComponent<icpMeshRendererComponent>();
			meshComp.m_meshResId = primitive.m_id;
			meshComp.m_meshVertexIndicesNum = primitive.m_meshData.m_vertexIndices.size();

			meshComp.prepareRenderResourceForMesh();
			meshComp.AddMaterial(primitive.m_pMaterial);

			thisGuid = entity->accessComponent<icpEntityDataComponent>().m_guid;
		}
	}

	for (int childIndex = 0; childIndex < node.children.size(); childIndex++)
	{
		LoadGLTFNode(gltfModel, node.children[childIndex], thisGuid, meshResources);
	}

}


INCEPTION_END_NAMESPACE

