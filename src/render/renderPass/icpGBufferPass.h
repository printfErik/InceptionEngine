#pragma once
#include "../../core/icpMacros.h"
#include "icpRenderPassBase.h"
#include "../../render/material/icpMaterial.h"
#include "../../scene/icpEntity.h"
#include "../../mesh/icpPrimitiveRendererComponent.h"

INCEPTION_BEGIN_NAMESPACE
class icpGBufferPass : public icpRenderPassBase
{
public:

	enum eGBufferPassDSType : uint8_t
	{
		PER_MESH = 0,
		PER_MATERIAL,
		LAYOUT_TYPE_COUNT
	};

	icpGBufferPass();
	virtual ~icpGBufferPass() override;

	void InitializeRenderPass(RenderPassInitInfo initInfo) override;
	void SetupPipeline() override;
	void SetupMaskedMeshPipeline();
	void Cleanup() override;
	void Render(uint32_t frameBufferIndex, uint32_t currentFrame, VkResult acquireImageResult) override;
	void BeginDeferredRenderingInfo(VkCommandBuffer cmdBuf, uint32_t imageIndex) override;
	void UpdateRenderPassCB(uint32_t curFrame) override;

	void RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex, uint32_t curFrame);
	void AllocatedRenderPassDescriptorSets() override {}
private:

	template<typename CompType>
	void Draw(std::shared_ptr<icpGameEntity> entity, VkCommandBuffer commandBuffer, uint32_t currentFrame)
	{
		if (entity->hasComponent<CompType>())
		{
			const auto& meshRender = entity->accessComponent<CompType>();

			if (meshRender.m_pMaterial->m_shadingModel != eMaterialShadingModel::PBR_LIT)
			{
				return;
			}

			if (meshRender.m_pMaterial->m_blendMode == eMaterialBlendMode::OPAQUE)
			{
				vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineInfo.m_pipeline);
			}
			else if (meshRender.m_pMaterial->m_blendMode == eMaterialBlendMode::MASK)
			{
				vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, maskedMeshPipeline.m_pipeline);
			}
			else
			{
				return;
			}

			VkDeviceSize offsets = 0;

			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
				m_pipelineInfo.m_pipelineLayout, 0, 1,
				&meshRender.MeshDSs[currentFrame], 0, nullptr);

			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
				m_pipelineInfo.m_pipelineLayout, 1, 1,
				&meshRender.m_pMaterial->MaterialDSs[currentFrame], 0, nullptr);

			vkCmdBindVertexBuffers(commandBuffer, 0, 1, &meshRender.MeshVB.buffer, &offsets);
			vkCmdBindIndexBuffer(commandBuffer, meshRender.MeshIB.buffer, 0, VK_INDEX_TYPE_UINT32);
			
			if constexpr (std::is_same_v<CompType, icpPrimitiveRendererComponent>)
			{
				vkCmdSetPrimitiveTopology(commandBuffer, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP);
			}
			else
			{
				vkCmdSetPrimitiveTopology(commandBuffer, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
			}
			vkCmdDrawIndexed(commandBuffer, meshRender.GetMeshIndexNum(), 1, 0, 0, 0);
		}
	}

	RenderPipelineInfo maskedMeshPipeline{};

};

INCEPTION_END_NAMESPACE