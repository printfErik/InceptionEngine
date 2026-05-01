#include "icpD3D12DeferredRenderer.h"

#include "../core/icpConfigSystem.h"
#include "../core/icpSystemContainer.h"
#include "../mesh/icpMeshRendererComponent.h"
#include "../mesh/icpPrimitiveRendererComponent.h"
#include "../scene/icpSceneSystem.h"
#include "../scene/icpXFormComponent.h"
#include "icpCameraSystem.h"
#include "light/icpLightSystem.h"

#include <glm/gtc/matrix_transform.hpp>
#include <cstring>

INCEPTION_BEGIN_NAMESPACE

namespace
{
static icpD3D12Texture* AsD3D12Texture(const std::shared_ptr<icpRHITexture>& texture)
{
	return static_cast<icpD3D12Texture*>(texture.get());
}

static icpD3D12Buffer* AsD3D12Buffer(const icpBufferRenderResource& buffer)
{
	return static_cast<icpD3D12Buffer*>(buffer.buffer.get());
}

static icpD3D12Pipeline* AsD3D12Pipeline(const std::shared_ptr<icpRHIPipeline>& pipeline)
{
	return static_cast<icpD3D12Pipeline*>(pipeline.get());
}
}

bool icpD3D12DeferredRenderer::Initialize(std::shared_ptr<icpGPUDevice> rhi)
{
	m_pDevice = rhi;
	m_d3dDevice = std::dynamic_pointer_cast<icpD3D12GPUDevice>(rhi);
	CreateSceneCB();
	CreateRenderTargets();
	CreatePipelines();
	return true;
}

void icpD3D12DeferredRenderer::Cleanup()
{
	m_gbufferPipeline.reset();
	m_compositePipeline.reset();
	m_gbufferA.reset();
	m_gbufferB.reset();
	m_gbufferC.reset();
	m_depth.reset();
}

void icpD3D12DeferredRenderer::CreateSceneCB()
{
	SceneUBOs.resize(MAX_FRAMES_IN_FLIGHT);
	for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
	{
		icpRHIBufferDesc desc{};
		desc.size = sizeof(perFrameCB);
		desc.usage = icpBufferUsage::UNIFORM;
		desc.debugName = "SceneCB";
		SceneUBOs[i].buffer = m_pDevice->CreateBuffer(desc);
		SceneUBOs[i].offset = 0;
		SceneUBOs[i].range = desc.size;
	}
}

void icpD3D12DeferredRenderer::CreateRenderTargets()
{
	icpRHITextureDesc rtDesc{};
	rtDesc.width = m_pDevice->GetBackBufferWidth();
	rtDesc.height = m_pDevice->GetBackBufferHeight();
	rtDesc.format = icpFormat::R16G16B16A16_FLOAT;
	rtDesc.usage = icpTextureUsage::RENDER_TARGET | icpTextureUsage::SAMPLED;
	rtDesc.initialState = icpResourceState::RENDER_TARGET;

	rtDesc.debugName = "GBufferA";
	m_gbufferA = m_pDevice->CreateTexture(rtDesc);
	rtDesc.debugName = "GBufferB";
	m_gbufferB = m_pDevice->CreateTexture(rtDesc);
	rtDesc.debugName = "GBufferC";
	m_gbufferC = m_pDevice->CreateTexture(rtDesc);

	icpRHITextureDesc depthDesc{};
	depthDesc.width = m_pDevice->GetBackBufferWidth();
	depthDesc.height = m_pDevice->GetBackBufferHeight();
	depthDesc.format = icpFormat::D32_FLOAT;
	depthDesc.usage = icpTextureUsage::DEPTH_STENCIL | icpTextureUsage::SAMPLED;
	depthDesc.initialState = icpResourceState::DEPTH_WRITE;
	depthDesc.debugName = "D3D12Depth";
	m_depth = m_pDevice->CreateTexture(depthDesc);

	CreateCompositeDescriptorTable();
	m_targetsValid = true;
}

void icpD3D12DeferredRenderer::CreateCompositeDescriptorTable()
{
	auto [cpuStart, gpuStart] = m_d3dDevice->AllocateSRV();
	m_compositeSRVGpu = gpuStart;

	auto cpu = cpuStart;
	const auto increment = m_d3dDevice->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	const std::shared_ptr<icpRHITexture> textures[] = { m_gbufferA, m_gbufferB, m_gbufferC, m_depth };
	for (uint32_t i = 0; i < 4; ++i)
	{
		if (i > 0)
		{
			cpu.ptr += increment;
			m_d3dDevice->AllocateSRV();
		}
		D3D12_SHADER_RESOURCE_VIEW_DESC srv{};
		srv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srv.Texture2D.MipLevels = 1;
		srv.Format = i == 3 ? DXGI_FORMAT_R32_FLOAT : DXGI_FORMAT_R16G16B16A16_FLOAT;
		m_d3dDevice->GetDevice()->CreateShaderResourceView(AsD3D12Texture(textures[i])->m_resource.Get(), &srv, cpu);
	}
}

void icpD3D12DeferredRenderer::CreatePipelines()
{
	icpGraphicsPipelineDesc gbufferDesc{};
	gbufferDesc.kind = icpPipelineKind::GBUFFER;
	gbufferDesc.vertexShader = g_system_container.m_configSystem->m_shaderFolderPath / "GBufferVS.cso";
	gbufferDesc.pixelShader = g_system_container.m_configSystem->m_shaderFolderPath / "GBufferPS.cso";
	gbufferDesc.vertexStride = sizeof(icpVertex);
	gbufferDesc.vertexAttributes = icpVertex::getAttributeDescription();
	gbufferDesc.renderTargetFormats = { icpFormat::R16G16B16A16_FLOAT, icpFormat::R16G16B16A16_FLOAT, icpFormat::R16G16B16A16_FLOAT };
	gbufferDesc.depthFormat = icpFormat::D32_FLOAT;
	gbufferDesc.depthTestEnable = true;
	gbufferDesc.depthWriteEnable = true;
	gbufferDesc.cullMode = icpCullMode::NONE;
	m_gbufferPipeline = m_pDevice->CreateGraphicsPipeline(gbufferDesc);

	icpGraphicsPipelineDesc compositeDesc{};
	compositeDesc.kind = icpPipelineKind::DEFERRED_COMPOSITE;
	compositeDesc.vertexShader = g_system_container.m_configSystem->m_shaderFolderPath / "DeferredCompositeVS.cso";
	compositeDesc.pixelShader = g_system_container.m_configSystem->m_shaderFolderPath / "DeferredCompositePS.cso";
	compositeDesc.renderTargetFormats = { icpFormat::R8G8B8A8_UNORM };
	compositeDesc.depthFormat = icpFormat::UNKNOWN;
	compositeDesc.depthTestEnable = false;
	compositeDesc.depthWriteEnable = false;
	compositeDesc.cullMode = icpCullMode::NONE;
	m_compositePipeline = m_pDevice->CreateGraphicsPipeline(compositeDesc);
}

void icpD3D12DeferredRenderer::Render()
{
	if (m_pDevice->m_framebufferResized)
	{
		m_pDevice->m_framebufferResized = false;
		m_pDevice->ResizeSwapchain();
		m_targetsValid = false;
	}

	if (!m_targetsValid ||
		m_pDevice->GetBackBufferWidth() != AsD3D12Texture(m_gbufferA)->m_width ||
		m_pDevice->GetBackBufferHeight() != AsD3D12Texture(m_gbufferA)->m_height)
	{
		CreateRenderTargets();
	}

	m_pDevice->BeginFrame();
	m_currentFrame = m_pDevice->GetCurrentFrameIndex();

	UpdateSceneCB(m_currentFrame);
	UpdateMeshes(m_currentFrame);

	auto* cmd = m_d3dDevice->GetCommandList();
	ID3D12DescriptorHeap* heaps[] = { m_d3dDevice->GetShaderVisibleHeap() };
	cmd->SetDescriptorHeaps(1, heaps);

	GBufferPass(cmd, m_currentFrame);
	CompositePass(cmd, m_currentFrame);

	m_pDevice->EndFrame();
}

void icpD3D12DeferredRenderer::UpdateSceneCB(uint32_t frameIndex)
{
	perFrameCB cb{};
	const float aspectRatio = static_cast<float>(m_pDevice->GetBackBufferWidth()) / static_cast<float>(m_pDevice->GetBackBufferHeight());
	g_system_container.m_cameraSystem->UpdateCameraCB(cb, aspectRatio);
	g_system_container.m_lightSystem->UpdateLightCB(cb);

	void* data = SceneUBOs[frameIndex].buffer->Map();
	memcpy(data, &cb, sizeof(cb));
	SceneUBOs[frameIndex].buffer->Unmap();
}

void icpD3D12DeferredRenderer::UpdateMeshes(uint32_t frameIndex)
{
	auto view = g_system_container.m_sceneSystem->m_registry.view<icpMeshRendererComponent, icpXFormComponent>();
	for (auto entity : view)
	{
		auto& mesh = view.get<icpMeshRendererComponent>(entity);
		if (!mesh.m_pMaterial || !mesh.m_pMaterial->m_bRenderResourcesReady)
		{
			continue;
		}
		auto& xform = view.get<icpXFormComponent>(entity);
		UBOMeshRenderResource ubo{};
		ubo.model = xform.m_mtxTransform;
		ubo.normalMatrix = glm::transpose(glm::inverse(glm::mat4(glm::mat3(ubo.model))));
		mesh.UploadMeshCB(ubo);
		mesh.UploadMaterialCB();
	}

	auto primitiveView = g_system_container.m_sceneSystem->m_registry.view<icpPrimitiveRendererComponent, icpXFormComponent>();
	for (auto entity : primitiveView)
	{
		auto& primitive = primitiveView.get<icpPrimitiveRendererComponent>(entity);
		if (!primitive.m_pMaterial || !primitive.m_pMaterial->m_bRenderResourcesReady)
		{
			continue;
		}
		auto& xform = primitiveView.get<icpXFormComponent>(entity);
		UBOMeshRenderResource ubo{};
		ubo.model = glm::translate(glm::mat4(1.f), xform.m_translation);
		ubo.model = glm::scale(ubo.model, xform.m_scale);
		ubo.normalMatrix = glm::transpose(glm::inverse(glm::mat4(glm::mat3(ubo.model))));
		primitive.UploadMeshCB(ubo);
		primitive.UploadMaterialCB();
	}
}

void icpD3D12DeferredRenderer::GBufferPass(ID3D12GraphicsCommandList* cmd, uint32_t frameIndex)
{
	auto* a = AsD3D12Texture(m_gbufferA);
	auto* b = AsD3D12Texture(m_gbufferB);
	auto* c = AsD3D12Texture(m_gbufferC);
	auto* depth = AsD3D12Texture(m_depth);

	m_d3dDevice->TransitionResource(a->m_resource.Get(), m_d3dDevice->ToD3D12State(a->m_state), D3D12_RESOURCE_STATE_RENDER_TARGET);
	m_d3dDevice->TransitionResource(b->m_resource.Get(), m_d3dDevice->ToD3D12State(b->m_state), D3D12_RESOURCE_STATE_RENDER_TARGET);
	m_d3dDevice->TransitionResource(c->m_resource.Get(), m_d3dDevice->ToD3D12State(c->m_state), D3D12_RESOURCE_STATE_RENDER_TARGET);
	m_d3dDevice->TransitionResource(depth->m_resource.Get(), m_d3dDevice->ToD3D12State(depth->m_state), D3D12_RESOURCE_STATE_DEPTH_WRITE);
	a->m_state = icpResourceState::RENDER_TARGET;
	b->m_state = icpResourceState::RENDER_TARGET;
	c->m_state = icpResourceState::RENDER_TARGET;
	depth->m_state = icpResourceState::DEPTH_WRITE;

	D3D12_CPU_DESCRIPTOR_HANDLE rtvs[] = { a->m_rtv, b->m_rtv, c->m_rtv };
	cmd->OMSetRenderTargets(3, rtvs, FALSE, &depth->m_dsv);
	const float clear[4] = { 0.f, 0.f, 0.f, 1.f };
	for (const auto& rtv : rtvs)
	{
		cmd->ClearRenderTargetView(rtv, clear, 0, nullptr);
	}
	cmd->ClearDepthStencilView(depth->m_dsv, D3D12_CLEAR_FLAG_DEPTH, 1.f, 0, 0, nullptr);

	D3D12_VIEWPORT viewport{ 0.f, 0.f, static_cast<float>(m_pDevice->GetBackBufferWidth()), static_cast<float>(m_pDevice->GetBackBufferHeight()), 0.f, 1.f };
	D3D12_RECT scissor{ 0, 0, static_cast<LONG>(m_pDevice->GetBackBufferWidth()), static_cast<LONG>(m_pDevice->GetBackBufferHeight()) };
	cmd->RSSetViewports(1, &viewport);
	cmd->RSSetScissorRects(1, &scissor);

	auto* pipeline = AsD3D12Pipeline(m_gbufferPipeline);
	cmd->SetPipelineState(pipeline->m_pipelineState.Get());
	cmd->SetGraphicsRootSignature(pipeline->m_rootSignature.Get());
	cmd->SetGraphicsRootConstantBufferView(3, AsD3D12Buffer(SceneUBOs[frameIndex])->GetGPUAddress());
	DrawScene(cmd, frameIndex);

	m_d3dDevice->TransitionResource(a->m_resource.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	m_d3dDevice->TransitionResource(b->m_resource.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	m_d3dDevice->TransitionResource(c->m_resource.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	m_d3dDevice->TransitionResource(depth->m_resource.Get(), D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	a->m_state = icpResourceState::SHADER_RESOURCE;
	b->m_state = icpResourceState::SHADER_RESOURCE;
	c->m_state = icpResourceState::SHADER_RESOURCE;
	depth->m_state = icpResourceState::SHADER_RESOURCE;
}

void icpD3D12DeferredRenderer::DrawScene(ID3D12GraphicsCommandList* cmd, uint32_t frameIndex)
{
	std::vector<std::shared_ptr<icpGameEntity>> roots;
	g_system_container.m_sceneSystem->getRootEntityList(roots);

	auto drawMesh = [&](auto& mesh)
	{
		if (!mesh.m_pMaterial || !mesh.m_pMaterial->m_bRenderResourcesReady ||
			mesh.m_pMaterial->m_shadingModel != eMaterialShadingModel::PBR_LIT ||
			mesh.m_pMaterial->m_blendMode == eMaterialBlendMode::TRANSLUCENT)
		{
			return;
		}

		cmd->SetGraphicsRootConstantBufferView(0, AsD3D12Buffer(mesh.MeshUBOs[frameIndex])->GetGPUAddress());
		cmd->SetGraphicsRootConstantBufferView(1, AsD3D12Buffer(mesh.m_pMaterial->MaterialUBOs[frameIndex])->GetGPUAddress());
		D3D12_GPU_DESCRIPTOR_HANDLE materialTable{};
		materialTable.ptr = mesh.m_pMaterial->m_srvTableGpuHandle;
		cmd->SetGraphicsRootDescriptorTable(2, materialTable);

		D3D12_VERTEX_BUFFER_VIEW vbv{};
		vbv.BufferLocation = AsD3D12Buffer(mesh.MeshVB)->GetGPUAddress();
		vbv.SizeInBytes = static_cast<UINT>(mesh.MeshVB.range);
		vbv.StrideInBytes = sizeof(icpVertex);
		D3D12_INDEX_BUFFER_VIEW ibv{};
		ibv.BufferLocation = AsD3D12Buffer(mesh.MeshIB)->GetGPUAddress();
		ibv.SizeInBytes = static_cast<UINT>(mesh.MeshIB.range);
		ibv.Format = DXGI_FORMAT_R32_UINT;

		cmd->IASetVertexBuffers(0, 1, &vbv);
		cmd->IASetIndexBuffer(&ibv);
		cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		cmd->DrawIndexedInstanced(mesh.GetMeshIndexNum(), 1, 0, 0, 0);
	};

	for (auto& entity : roots)
	{
		if (entity->hasComponent<icpMeshRendererComponent>())
		{
			drawMesh(entity->accessComponent<icpMeshRendererComponent>());
		}
		if (entity->hasComponent<icpPrimitiveRendererComponent>())
		{
			drawMesh(entity->accessComponent<icpPrimitiveRendererComponent>());
		}
	}
}

void icpD3D12DeferredRenderer::CompositePass(ID3D12GraphicsCommandList* cmd, uint32_t frameIndex)
{
	auto* backBuffer = m_d3dDevice->GetCurrentBackBuffer();
	m_d3dDevice->TransitionResource(backBuffer, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

	auto backBufferRTV = m_d3dDevice->GetCurrentBackBufferRTV();
	cmd->OMSetRenderTargets(1, &backBufferRTV, FALSE, nullptr);
	const float clear[4] = { 0.f, 0.f, 0.f, 1.f };
	cmd->ClearRenderTargetView(backBufferRTV, clear, 0, nullptr);

	D3D12_VIEWPORT viewport{ 0.f, 0.f, static_cast<float>(m_pDevice->GetBackBufferWidth()), static_cast<float>(m_pDevice->GetBackBufferHeight()), 0.f, 1.f };
	D3D12_RECT scissor{ 0, 0, static_cast<LONG>(m_pDevice->GetBackBufferWidth()), static_cast<LONG>(m_pDevice->GetBackBufferHeight()) };
	cmd->RSSetViewports(1, &viewport);
	cmd->RSSetScissorRects(1, &scissor);

	auto* pipeline = AsD3D12Pipeline(m_compositePipeline);
	cmd->SetPipelineState(pipeline->m_pipelineState.Get());
	cmd->SetGraphicsRootSignature(pipeline->m_rootSignature.Get());
	cmd->SetGraphicsRootDescriptorTable(0, m_compositeSRVGpu);
	cmd->SetGraphicsRootConstantBufferView(1, AsD3D12Buffer(SceneUBOs[frameIndex])->GetGPUAddress());
	cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	cmd->DrawInstanced(3, 1, 0, 0);

	m_d3dDevice->TransitionResource(backBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
}

INCEPTION_END_NAMESPACE
