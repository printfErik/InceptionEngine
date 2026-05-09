#include "icpDeferredRenderer.h"

#include "../core/icpConfigSystem.h"
#include "../core/icpSystemContainer.h"
#include "../mesh/icpMeshRendererComponent.h"
#include "../mesh/icpPrimitiveRendererComponent.h"
#include "../scene/icpSceneSystem.h"
#include "../scene/icpXFormComponent.h"
#include "../ui/editorUI/icpEditorUI.h"
#include "icpCameraSystem.h"
#include "light/icpLightSystem.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <imgui.h>

INCEPTION_BEGIN_NAMESPACE

namespace
{
static glm::mat4 MakeDeferredOrtho(float left, float right, float bottom, float top, float nearPlane, float farPlane)
{
	return glm::orthoLH_ZO(left, right, bottom, top, nearPlane, farPlane);
}
}

icpDeferredRenderer::~icpDeferredRenderer()
{
	Cleanup();
}

bool icpDeferredRenderer::Initialize(std::shared_ptr<icpGPUDevice> rhi)
{
	m_pDevice = rhi;
	CreateSceneCB();
	CreateCSMCB();
	CreateCSMResources();
	CreateRenderTargets();
	CreatePipelines();
	InitializeImGui();
	return true;
}

void icpDeferredRenderer::Cleanup()
{
	ShutdownImGui();
	m_gbufferPipeline.reset();
	m_compositePipeline.reset();
	m_csmPipeline.reset();
	m_gtaoPipeline.reset();
	m_translucentPipeline.reset();
	m_gbufferA.reset();
	m_gbufferB.reset();
	m_gbufferC.reset();
	m_depth.reset();
	m_gtao.reset();
	m_compositeBindingSet.reset();
	m_gtaoInputBindingSet.reset();
	m_gtaoOutputBindingSet.reset();
	for (auto& shadowMap : m_shadowMaps)
	{
		shadowMap.reset();
	}
}

void icpDeferredRenderer::CreateSceneCB()
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

void icpDeferredRenderer::CreateCSMCB()
{
	m_csmUBOs.resize(MAX_FRAMES_IN_FLIGHT);
	for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
	{
		icpRHIBufferDesc desc{};
		desc.size = sizeof(icpCSMCB);
		desc.usage = icpBufferUsage::UNIFORM;
		desc.debugName = "CSMCB";
		m_csmUBOs[i].buffer = m_pDevice->CreateBuffer(desc);
		m_csmUBOs[i].offset = 0;
		m_csmUBOs[i].range = desc.size;
	}
}

void icpDeferredRenderer::CreateCSMResources()
{
	for (auto& shadowMap : m_shadowMaps)
	{
		icpRHITextureDesc desc{};
		desc.width = DEFERRED_CSM_RESOLUTION;
		desc.height = DEFERRED_CSM_RESOLUTION;
		desc.format = icpFormat::D32_FLOAT;
		desc.usage = icpTextureUsage::DEPTH_STENCIL | icpTextureUsage::SAMPLED;
		desc.initialState = icpResourceState::SHADER_RESOURCE;
		desc.debugName = "CSMShadowMap";
		shadowMap = m_pDevice->CreateTexture(desc);
	}
}

void icpDeferredRenderer::CreateRenderTargets()
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
	depthDesc.debugName = "DeferredDepth";
	m_depth = m_pDevice->CreateTexture(depthDesc);

	icpRHITextureDesc gtaoDesc{};
	gtaoDesc.width = m_pDevice->GetBackBufferWidth();
	gtaoDesc.height = m_pDevice->GetBackBufferHeight();
	gtaoDesc.format = icpFormat::R32_FLOAT;
	gtaoDesc.usage = icpTextureUsage::SAMPLED | icpTextureUsage::STORAGE;
	gtaoDesc.initialState = icpResourceState::SHADER_RESOURCE;
	gtaoDesc.debugName = "GTAO";
	m_gtao = m_pDevice->CreateTexture(gtaoDesc);

	CreateBindingSets();
	m_targetsValid = true;
}

void icpDeferredRenderer::CreateBindingSets()
{
	icpRHIBindingSetDesc compositeDesc{};
	compositeDesc.debugName = "DeferredCompositeInputs";
	compositeDesc.resources = {
		{ m_gbufferA, icpRHIResourceViewType::SRV },
		{ m_gbufferB, icpRHIResourceViewType::SRV },
		{ m_gbufferC, icpRHIResourceViewType::SRV },
		{ m_depth, icpRHIResourceViewType::SRV },
	};
	for (const auto& shadowMap : m_shadowMaps)
	{
		compositeDesc.resources.push_back({ shadowMap, icpRHIResourceViewType::SRV });
	}
	compositeDesc.resources.push_back({ m_gtao, icpRHIResourceViewType::SRV });
	m_compositeBindingSet = m_pDevice->CreateBindingSet(compositeDesc);

	icpRHIBindingSetDesc gtaoInputDesc{};
	gtaoInputDesc.debugName = "GTAOInputs";
	gtaoInputDesc.resources = {
		{ m_gbufferB, icpRHIResourceViewType::SRV },
		{ m_depth, icpRHIResourceViewType::SRV },
	};
	m_gtaoInputBindingSet = m_pDevice->CreateBindingSet(gtaoInputDesc);

	icpRHIBindingSetDesc gtaoOutputDesc{};
	gtaoOutputDesc.debugName = "GTAOOutput";
	gtaoOutputDesc.resources = {
		{ m_gtao, icpRHIResourceViewType::UAV },
	};
	m_gtaoOutputBindingSet = m_pDevice->CreateBindingSet(gtaoOutputDesc);
}

void icpDeferredRenderer::CreatePipelines()
{
	icpGraphicsPipelineDesc csmDesc{};
	csmDesc.kind = icpPipelineKind::CSM;
	csmDesc.vertexShader = g_system_container.m_configSystem->m_shaderFolderPath / "CSMVS.cso";
	csmDesc.pixelShader = g_system_container.m_configSystem->m_shaderFolderPath / "CSMPS.cso";
	csmDesc.vertexStride = sizeof(icpVertex);
	csmDesc.vertexAttributes = icpVertex::getAttributeDescription();
	csmDesc.depthFormat = icpFormat::D32_FLOAT;
	csmDesc.depthTestEnable = true;
	csmDesc.depthWriteEnable = true;
	csmDesc.cullMode = icpCullMode::NONE;
	m_csmPipeline = m_pDevice->CreateGraphicsPipeline(csmDesc);

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
	compositeDesc.depthTestEnable = false;
	compositeDesc.depthWriteEnable = false;
	compositeDesc.cullMode = icpCullMode::NONE;
	m_compositePipeline = m_pDevice->CreateGraphicsPipeline(compositeDesc);

	icpComputePipelineDesc gtaoDesc{};
	gtaoDesc.kind = icpPipelineKind::GTAO;
	gtaoDesc.computeShader = g_system_container.m_configSystem->m_shaderFolderPath / "GTAOCS.cso";
	m_gtaoPipeline = m_pDevice->CreateComputePipeline(gtaoDesc);

	icpGraphicsPipelineDesc translucentDesc{};
	translucentDesc.kind = icpPipelineKind::FORWARD_TRANSLUCENT;
	translucentDesc.vertexShader = g_system_container.m_configSystem->m_shaderFolderPath / "TranslucentVS.cso";
	translucentDesc.pixelShader = g_system_container.m_configSystem->m_shaderFolderPath / "TranslucentPS.cso";
	translucentDesc.vertexStride = sizeof(icpVertex);
	translucentDesc.vertexAttributes = icpVertex::getAttributeDescription();
	translucentDesc.renderTargetFormats = { icpFormat::R8G8B8A8_UNORM };
	translucentDesc.depthFormat = icpFormat::D32_FLOAT;
	translucentDesc.depthTestEnable = true;
	translucentDesc.depthWriteEnable = false;
	translucentDesc.cullMode = icpCullMode::BACK;
	translucentDesc.blendMode = icpBlendMode::TRANSLUCENT;
	m_translucentPipeline = m_pDevice->CreateGraphicsPipeline(translucentDesc);
}

void icpDeferredRenderer::InitializeImGui()
{
	if (m_imguiInitialized)
	{
		return;
	}

	m_pDevice->InitializeImGui(g_system_container.m_windowSystem);
	m_editorUI = std::make_shared<icpEditorUI>();
	m_imguiInitialized = true;
}

void icpDeferredRenderer::ShutdownImGui()
{
	if (!m_imguiInitialized)
	{
		return;
	}

	m_editorUI.reset();
	m_pDevice->ShutdownImGui();
	m_imguiInitialized = false;
}

void icpDeferredRenderer::Render()
{
	if (m_pDevice->m_framebufferResized)
	{
		m_pDevice->m_framebufferResized = false;
		m_pDevice->ResizeSwapchain();
		m_targetsValid = false;
	}

	if (!m_targetsValid ||
		m_pDevice->GetBackBufferWidth() != m_gbufferA->m_width ||
		m_pDevice->GetBackBufferHeight() != m_gbufferA->m_height)
	{
		CreateRenderTargets();
	}

	m_pDevice->BeginFrame();
	m_currentFrame = m_pDevice->GetCurrentFrameIndex();

	UpdateSceneCB(m_currentFrame);
	UpdateMeshes(m_currentFrame);

	auto commandList = m_pDevice->GetGraphicsCommandList();
	m_pDevice->PrepareCommandList(commandList);

	ShadowPass(commandList, m_currentFrame);
	GBufferPass(commandList, m_currentFrame);
	if (m_enableGTAO)
	{
		m_pDevice->SubmitGraphicsWorkBeforeAsyncCompute();
		const uint64_t gtaoFence = GTAOPass(m_currentFrame);
		m_pDevice->WaitForAsyncCompute(gtaoFence);

		commandList = m_pDevice->GetGraphicsCommandList();
		m_pDevice->PrepareCommandList(commandList);
		m_pDevice->TransitionTexture(commandList, m_gbufferB, icpResourceState::SHADER_RESOURCE);
		m_pDevice->TransitionTexture(commandList, m_depth, icpResourceState::SHADER_RESOURCE);
		m_pDevice->TransitionTexture(commandList, m_gtao, icpResourceState::SHADER_RESOURCE);
	}
	CompositePass(commandList, m_currentFrame);
	ForwardTranslucentPass(commandList, m_currentFrame);
	RenderImGui(commandList);
	m_pDevice->TransitionBackBuffer(commandList, icpResourceState::PRESENT);

	m_pDevice->EndFrame();
}

void icpDeferredRenderer::UpdateSceneCB(uint32_t frameIndex)
{
	const float aspectRatio = static_cast<float>(m_pDevice->GetBackBufferWidth()) / static_cast<float>(m_pDevice->GetBackBufferHeight());
	g_system_container.m_cameraSystem->UpdateCameraCB(m_frameCB, aspectRatio);
	g_system_container.m_lightSystem->UpdateLightCB(m_frameCB);
	UpdateCSMCB(frameIndex);

	void* data = SceneUBOs[frameIndex].buffer->Map();
	memcpy(data, &m_frameCB, sizeof(m_frameCB));
	SceneUBOs[frameIndex].buffer->Unmap();
}

void icpDeferredRenderer::UpdateCSMCB(uint32_t frameIndex)
{
	const float nearPlane = g_system_container.m_configSystem->NearPlane;
	const float farPlane = g_system_container.m_configSystem->FarPlane;
	float cascadeSplits[DEFERRED_CSM_CASCADE_COUNT + 1]{};
	cascadeSplits[0] = nearPlane;
	cascadeSplits[DEFERRED_CSM_CASCADE_COUNT] = farPlane;
	for (uint32_t i = 1; i < DEFERRED_CSM_CASCADE_COUNT; ++i)
	{
		const float si = static_cast<float>(i) / static_cast<float>(DEFERRED_CSM_CASCADE_COUNT);
		const float logSplit = nearPlane * std::pow(farPlane / nearPlane, si);
		const float linSplit = nearPlane + (farPlane - nearPlane) * si;
		cascadeSplits[i] = linSplit + 0.5f * (logSplit - linSplit);
	}
	m_csmData.cascadeSplits = glm::vec4(cascadeSplits[1], cascadeSplits[2], cascadeSplits[3], cascadeSplits[4]);
	m_csmData.renderOptions = glm::vec4(m_enableGTAO ? 1.f : 0.f, 0.f, 0.f, 0.f);

	auto camera = g_system_container.m_cameraSystem->getCurrentCamera();
	const float aspectRatio = static_cast<float>(m_pDevice->GetBackBufferWidth()) / static_cast<float>(m_pDevice->GetBackBufferHeight());
	const glm::mat4 invView = glm::inverse(m_frameCB.view);
	glm::vec3 lightDir = glm::normalize(glm::vec3(m_frameCB.dirLight.direction));
	if (glm::length(lightDir) < 0.001f)
	{
		lightDir = glm::normalize(glm::vec3(-1.f, -1.f, -1.f));
	}

	for (uint32_t cascade = 0; cascade < DEFERRED_CSM_CASCADE_COUNT; ++cascade)
	{
		const float nearZ = cascadeSplits[cascade];
		const float farZ = cascadeSplits[cascade + 1];
		const float tanHalfFov = std::tan(camera->m_fov * 0.5f);
		const float nearH = tanHalfFov * nearZ;
		const float nearW = nearH * aspectRatio;
		const float farH = tanHalfFov * farZ;
		const float farW = farH * aspectRatio;

		const glm::vec4 cornersVS[] = {
			{ nearW, nearH, -nearZ, 1.f }, { -nearW, nearH, -nearZ, 1.f },
			{ -nearW, -nearH, -nearZ, 1.f }, { nearW, -nearH, -nearZ, 1.f },
			{ farW, farH, -farZ, 1.f }, { -farW, farH, -farZ, 1.f },
			{ -farW, -farH, -farZ, 1.f }, { farW, -farH, -farZ, 1.f },
		};

		glm::vec3 cornersWS[8]{};
		glm::vec3 center(0.f);
		for (uint32_t i = 0; i < 8; ++i)
		{
			glm::vec4 corner = invView * cornersVS[i];
			cornersWS[i] = glm::vec3(corner) / corner.w;
			center += cornersWS[i];
		}
		center /= 8.f;

		float radius = 0.f;
		for (const auto& corner : cornersWS)
		{
			radius = (std::max)(radius, glm::length(corner - center));
		}
		radius = std::ceil(radius * 16.f) / 16.f;

		const glm::vec3 eye = center - lightDir * radius;
		glm::vec3 up(0.f, 1.f, 0.f);
		if (std::abs(glm::dot(up, lightDir)) > 0.95f)
		{
			up = glm::vec3(0.f, 0.f, 1.f);
		}

		const glm::mat4 lightView = glm::lookAtLH(eye, center, up);
		const glm::mat4 lightProj = MakeDeferredOrtho(-radius, radius, -radius, radius, 0.f, radius * 2.f);
		m_csmData.lightViewProj[cascade] = lightProj * lightView;
	}

	void* data = m_csmUBOs[frameIndex].buffer->Map();
	memcpy(data, &m_csmData, sizeof(m_csmData));
	m_csmUBOs[frameIndex].buffer->Unmap();
}

void icpDeferredRenderer::UpdateMeshes(uint32_t frameIndex)
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

void icpDeferredRenderer::ShadowPass(std::shared_ptr<icpRHICommandList> commandList, uint32_t frameIndex)
{
	m_pDevice->BindGraphicsPipeline(commandList, m_csmPipeline);
	m_pDevice->BindGraphicsConstantBuffer(commandList, 1, m_csmUBOs[frameIndex].buffer);
	m_pDevice->SetViewportAndScissor(commandList, DEFERRED_CSM_RESOLUTION, DEFERRED_CSM_RESOLUTION);

	for (uint32_t cascade = 0; cascade < DEFERRED_CSM_CASCADE_COUNT; ++cascade)
	{
		m_pDevice->TransitionTexture(commandList, m_shadowMaps[cascade], icpResourceState::DEPTH_WRITE);
		m_pDevice->SetRenderTargets(commandList, {}, m_shadowMaps[cascade], icpRHIDepthAccess::WRITE, false, true);
		m_pDevice->SetGraphicsConstant(commandList, 2, cascade);
		DrawShadowScene(commandList, frameIndex, cascade);
		m_pDevice->TransitionTexture(commandList, m_shadowMaps[cascade], icpResourceState::SHADER_RESOURCE);
	}
}

void icpDeferredRenderer::GBufferPass(std::shared_ptr<icpRHICommandList> commandList, uint32_t frameIndex)
{
	m_pDevice->TransitionTexture(commandList, m_gbufferA, icpResourceState::RENDER_TARGET);
	m_pDevice->TransitionTexture(commandList, m_gbufferB, icpResourceState::RENDER_TARGET);
	m_pDevice->TransitionTexture(commandList, m_gbufferC, icpResourceState::RENDER_TARGET);
	m_pDevice->TransitionTexture(commandList, m_depth, icpResourceState::DEPTH_WRITE);
	m_pDevice->SetRenderTargets(commandList, { m_gbufferA, m_gbufferB, m_gbufferC }, m_depth, icpRHIDepthAccess::WRITE, true, true);
	m_pDevice->SetViewportAndScissor(commandList, m_pDevice->GetBackBufferWidth(), m_pDevice->GetBackBufferHeight());
	m_pDevice->BindGraphicsPipeline(commandList, m_gbufferPipeline);
	m_pDevice->BindGraphicsConstantBuffer(commandList, 3, SceneUBOs[frameIndex].buffer);
	DrawScene(commandList, frameIndex);

	m_pDevice->TransitionTexture(commandList, m_gbufferA, icpResourceState::SHADER_RESOURCE);
	m_pDevice->TransitionTexture(commandList, m_gbufferC, icpResourceState::SHADER_RESOURCE);
	if (m_enableGTAO)
	{
		m_pDevice->TransitionTexture(commandList, m_gbufferB, icpResourceState::NON_PIXEL_SHADER_RESOURCE);
		m_pDevice->TransitionTexture(commandList, m_depth, icpResourceState::NON_PIXEL_SHADER_RESOURCE);
		m_pDevice->TransitionTexture(commandList, m_gtao, icpResourceState::UNORDERED_ACCESS);
	}
	else
	{
		m_pDevice->TransitionTexture(commandList, m_gbufferB, icpResourceState::SHADER_RESOURCE);
		m_pDevice->TransitionTexture(commandList, m_depth, icpResourceState::SHADER_RESOURCE);
	}
}

void icpDeferredRenderer::DrawScene(std::shared_ptr<icpRHICommandList> commandList, uint32_t frameIndex)
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

		m_pDevice->BindGraphicsConstantBuffer(commandList, 0, mesh.MeshUBOs[frameIndex].buffer);
		m_pDevice->BindGraphicsConstantBuffer(commandList, 1, mesh.m_pMaterial->MaterialUBOs[frameIndex].buffer);
		m_pDevice->BindGraphicsBindingSet(commandList, 2, mesh.m_pMaterial->m_textureBindingSet);
		m_pDevice->BindVertexAndIndexBuffers(
			commandList,
			mesh.MeshVB.buffer,
			mesh.MeshVB.range,
			mesh.MeshIB.buffer,
			mesh.MeshIB.range,
			sizeof(icpVertex));
		m_pDevice->DrawIndexed(commandList, mesh.GetMeshIndexNum());
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

uint64_t icpDeferredRenderer::GTAOPass(uint32_t frameIndex)
{
	if (!m_gtaoPipeline || !m_gtao || !m_pDevice->SupportsAsyncCompute())
	{
		return 0;
	}

	auto computeList = m_pDevice->BeginAsyncCompute();
	m_pDevice->PrepareCommandList(computeList);
	m_pDevice->BindComputePipeline(computeList, m_gtaoPipeline);
	m_pDevice->BindComputeBindingSet(computeList, 0, m_gtaoInputBindingSet);
	m_pDevice->BindComputeBindingSet(computeList, 1, m_gtaoOutputBindingSet);
	m_pDevice->BindComputeConstantBuffer(computeList, 2, SceneUBOs[frameIndex].buffer);
	const uint32_t groupsX = (m_pDevice->GetBackBufferWidth() + 15u) / 16u;
	const uint32_t groupsY = (m_pDevice->GetBackBufferHeight() + 15u) / 16u;
	m_pDevice->Dispatch(computeList, groupsX, groupsY, 1);

	return m_pDevice->EndAsyncCompute(computeList);
}

void icpDeferredRenderer::DrawShadowScene(std::shared_ptr<icpRHICommandList> commandList, uint32_t frameIndex, uint32_t cascadeIndex)
{
	(void)cascadeIndex;
	std::vector<std::shared_ptr<icpGameEntity>> roots;
	g_system_container.m_sceneSystem->getRootEntityList(roots);

	auto drawMesh = [&](auto& mesh)
	{
		if (!mesh.m_pMaterial || !mesh.m_pMaterial->m_bRenderResourcesReady ||
			mesh.m_pMaterial->m_blendMode == eMaterialBlendMode::TRANSLUCENT)
		{
			return;
		}

		m_pDevice->BindGraphicsConstantBuffer(commandList, 0, mesh.MeshUBOs[frameIndex].buffer);
		m_pDevice->BindVertexAndIndexBuffers(
			commandList,
			mesh.MeshVB.buffer,
			mesh.MeshVB.range,
			mesh.MeshIB.buffer,
			mesh.MeshIB.range,
			sizeof(icpVertex));
		m_pDevice->DrawIndexed(commandList, mesh.GetMeshIndexNum());
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

void icpDeferredRenderer::CompositePass(std::shared_ptr<icpRHICommandList> commandList, uint32_t frameIndex)
{
	m_pDevice->TransitionBackBuffer(commandList, icpResourceState::RENDER_TARGET);
	m_pDevice->SetBackBufferRenderTarget(commandList, true);
	m_pDevice->SetViewportAndScissor(commandList, m_pDevice->GetBackBufferWidth(), m_pDevice->GetBackBufferHeight());
	m_pDevice->BindGraphicsPipeline(commandList, m_compositePipeline);
	m_pDevice->BindGraphicsBindingSet(commandList, 0, m_compositeBindingSet);
	m_pDevice->BindGraphicsConstantBuffer(commandList, 1, SceneUBOs[frameIndex].buffer);
	m_pDevice->BindGraphicsConstantBuffer(commandList, 2, m_csmUBOs[frameIndex].buffer);
	m_pDevice->Draw(commandList, 3);
}

void icpDeferredRenderer::ForwardTranslucentPass(std::shared_ptr<icpRHICommandList> commandList, uint32_t frameIndex)
{
	m_pDevice->TransitionTexture(commandList, m_depth, icpResourceState::DEPTH_READ);
	m_pDevice->SetBackBufferRenderTarget(commandList, m_depth, icpRHIDepthAccess::READ, false);
	m_pDevice->SetViewportAndScissor(commandList, m_pDevice->GetBackBufferWidth(), m_pDevice->GetBackBufferHeight());
	m_pDevice->BindGraphicsPipeline(commandList, m_translucentPipeline);
	m_pDevice->BindGraphicsConstantBuffer(commandList, 3, SceneUBOs[frameIndex].buffer);
	DrawTranslucentScene(commandList, frameIndex);
	m_pDevice->TransitionTexture(commandList, m_depth, icpResourceState::SHADER_RESOURCE);
}

void icpDeferredRenderer::DrawTranslucentScene(std::shared_ptr<icpRHICommandList> commandList, uint32_t frameIndex)
{
	std::vector<std::shared_ptr<icpGameEntity>> roots;
	g_system_container.m_sceneSystem->getRootEntityList(roots);

	auto drawMesh = [&](auto& mesh)
	{
		if (!mesh.m_pMaterial || !mesh.m_pMaterial->m_bRenderResourcesReady ||
			mesh.m_pMaterial->m_shadingModel != eMaterialShadingModel::PBR_LIT ||
			mesh.m_pMaterial->m_blendMode != eMaterialBlendMode::TRANSLUCENT)
		{
			return;
		}

		m_pDevice->BindGraphicsConstantBuffer(commandList, 0, mesh.MeshUBOs[frameIndex].buffer);
		m_pDevice->BindGraphicsConstantBuffer(commandList, 1, mesh.m_pMaterial->MaterialUBOs[frameIndex].buffer);
		m_pDevice->BindGraphicsBindingSet(commandList, 2, mesh.m_pMaterial->m_textureBindingSet);
		m_pDevice->BindVertexAndIndexBuffers(
			commandList,
			mesh.MeshVB.buffer,
			mesh.MeshVB.range,
			mesh.MeshIB.buffer,
			mesh.MeshIB.range,
			sizeof(icpVertex));
		m_pDevice->DrawIndexed(commandList, mesh.GetMeshIndexNum());
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

void icpDeferredRenderer::RenderImGui(std::shared_ptr<icpRHICommandList> commandList)
{
	if (!m_imguiInitialized || !m_editorUI)
	{
		return;
	}

	m_pDevice->BeginImGuiFrame();
	m_editorUI->showEditorUI();
	ImGui::Render();
	m_pDevice->RenderImGuiDrawData(commandList);
}

INCEPTION_END_NAMESPACE
