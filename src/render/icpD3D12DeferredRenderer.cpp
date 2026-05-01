#include "icpD3D12DeferredRenderer.h"

#include "../core/icpConfigSystem.h"
#include "../core/icpSystemContainer.h"
#include "../mesh/icpMeshRendererComponent.h"
#include "../mesh/icpPrimitiveRendererComponent.h"
#include "../scene/icpSceneSystem.h"
#include "../scene/icpXFormComponent.h"
#include "icpCameraSystem.h"
#include "light/icpLightSystem.h"
#include "../ui/editorUI/icpEditorUI.h"

#include <backends/imgui_impl_dx12.h>
#include <backends/imgui_impl_glfw.h>
#include <imgui.h>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cstring>
#include <cmath>

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

static glm::mat4 MakeD3D12Ortho(float left, float right, float bottom, float top, float nearPlane, float farPlane)
{
	return glm::orthoLH_ZO(left, right, bottom, top, nearPlane, farPlane);
}
}

icpD3D12DeferredRenderer::~icpD3D12DeferredRenderer()
{
	Cleanup();
}

bool icpD3D12DeferredRenderer::Initialize(std::shared_ptr<icpGPUDevice> rhi)
{
	m_pDevice = rhi;
	m_d3dDevice = std::dynamic_pointer_cast<icpD3D12GPUDevice>(rhi);
	CreateSceneCB();
	CreateCSMCB();
	CreateCSMResources();
	CreateRenderTargets();
	CreatePipelines();
	InitializeImGui();
	return true;
}

void icpD3D12DeferredRenderer::Cleanup()
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
	for (auto& shadowMap : m_shadowMaps)
	{
		shadowMap.reset();
	}
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

void icpD3D12DeferredRenderer::CreateCSMCB()
{
	m_csmUBOs.resize(MAX_FRAMES_IN_FLIGHT);
	for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
	{
		icpRHIBufferDesc desc{};
		desc.size = sizeof(icpD3D12CSMCB);
		desc.usage = icpBufferUsage::UNIFORM;
		desc.debugName = "CSMCB";
		m_csmUBOs[i].buffer = m_pDevice->CreateBuffer(desc);
		m_csmUBOs[i].offset = 0;
		m_csmUBOs[i].range = desc.size;
	}
}

void icpD3D12DeferredRenderer::CreateCSMResources()
{
	for (uint32_t i = 0; i < D3D12_CSM_CASCADE_COUNT; ++i)
	{
		icpRHITextureDesc desc{};
		desc.width = D3D12_CSM_RESOLUTION;
		desc.height = D3D12_CSM_RESOLUTION;
		desc.format = icpFormat::D32_FLOAT;
		desc.usage = icpTextureUsage::DEPTH_STENCIL | icpTextureUsage::SAMPLED;
		desc.initialState = icpResourceState::SHADER_RESOURCE;
		desc.debugName = "CSMShadowMap";
		m_shadowMaps[i] = m_pDevice->CreateTexture(desc);
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

	icpRHITextureDesc gtaoDesc{};
	gtaoDesc.width = m_pDevice->GetBackBufferWidth();
	gtaoDesc.height = m_pDevice->GetBackBufferHeight();
	gtaoDesc.format = icpFormat::R32_FLOAT;
	gtaoDesc.usage = icpTextureUsage::SAMPLED | icpTextureUsage::STORAGE;
	gtaoDesc.initialState = icpResourceState::SHADER_RESOURCE;
	gtaoDesc.debugName = "GTAO";
	m_gtao = m_pDevice->CreateTexture(gtaoDesc);

	CreateCompositeDescriptorTable();
	CreateGTAODescriptorTable();
	m_targetsValid = true;
}

void icpD3D12DeferredRenderer::CreateCompositeDescriptorTable()
{
	auto [cpuStart, gpuStart] = m_d3dDevice->AllocateSRV();
	m_compositeSRVGpu = gpuStart;

	auto cpu = cpuStart;
	const auto increment = m_d3dDevice->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	const std::shared_ptr<icpRHITexture> textures[] = {
		m_gbufferA, m_gbufferB, m_gbufferC, m_depth,
		m_shadowMaps[0], m_shadowMaps[1], m_shadowMaps[2], m_shadowMaps[3],
		m_gtao
	};
	for (uint32_t i = 0; i < 9; ++i)
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
		srv.Format = (i == 3 || i >= 4) ? DXGI_FORMAT_R32_FLOAT : DXGI_FORMAT_R16G16B16A16_FLOAT;
		m_d3dDevice->GetDevice()->CreateShaderResourceView(AsD3D12Texture(textures[i])->m_resource.Get(), &srv, cpu);
	}
}

void icpD3D12DeferredRenderer::CreateGTAODescriptorTable()
{
	auto [cpuStart, gpuStart] = m_d3dDevice->AllocateSRV();
	m_gtaoSRVGpu = gpuStart;

	auto cpu = cpuStart;
	const auto increment = m_d3dDevice->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	const std::shared_ptr<icpRHITexture> textures[] = { m_gbufferB, m_depth };
	for (uint32_t i = 0; i < 2; ++i)
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
		srv.Format = i == 0 ? DXGI_FORMAT_R16G16B16A16_FLOAT : DXGI_FORMAT_R32_FLOAT;
		m_d3dDevice->GetDevice()->CreateShaderResourceView(AsD3D12Texture(textures[i])->m_resource.Get(), &srv, cpu);
	}
}

void icpD3D12DeferredRenderer::CreatePipelines()
{
	icpGraphicsPipelineDesc csmDesc{};
	csmDesc.kind = icpPipelineKind::CSM;
	csmDesc.vertexShader = g_system_container.m_configSystem->m_shaderFolderPath / "CSMVS.cso";
	csmDesc.pixelShader = g_system_container.m_configSystem->m_shaderFolderPath / "CSMPS.cso";
	csmDesc.vertexStride = sizeof(icpVertex);
	csmDesc.vertexAttributes = icpVertex::getAttributeDescription();
	csmDesc.renderTargetFormats = {};
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
	compositeDesc.depthFormat = icpFormat::UNKNOWN;
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

void icpD3D12DeferredRenderer::InitializeImGui()
{
	if (m_imguiInitialized)
	{
		return;
	}

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

	ImGui_ImplGlfw_InitForOther(g_system_container.m_windowSystem->getWindow(), true);
	auto [fontCpu, fontGpu] = m_d3dDevice->AllocateSRV();
	m_imguiFontSRVCpu = fontCpu;
	m_imguiFontSRVGpu = fontGpu;
	ImGui_ImplDX12_Init(
		m_d3dDevice->GetDevice(),
		MAX_FRAMES_IN_FLIGHT,
		DXGI_FORMAT_R8G8B8A8_UNORM,
		m_d3dDevice->GetShaderVisibleHeap(),
		m_imguiFontSRVCpu,
		m_imguiFontSRVGpu);

	m_editorUI = std::make_shared<icpEditorUI>();
	m_imguiInitialized = true;
}

void icpD3D12DeferredRenderer::ShutdownImGui()
{
	if (!m_imguiInitialized)
	{
		return;
	}

	ImGui_ImplDX12_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
	m_editorUI.reset();
	m_imguiInitialized = false;
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

	ShadowPass(cmd, m_currentFrame);
	GBufferPass(cmd, m_currentFrame);
	if (m_enableGTAO)
	{
		m_pDevice->SubmitGraphicsWorkBeforeAsyncCompute();
		const uint64_t gtaoFence = GTAOPass(m_currentFrame);
		m_pDevice->WaitForAsyncCompute(gtaoFence);

		cmd = m_d3dDevice->GetCommandList();
		cmd->SetDescriptorHeaps(1, heaps);
		auto* gbufferB = AsD3D12Texture(m_gbufferB);
		auto* depth = AsD3D12Texture(m_depth);
		auto* gtao = AsD3D12Texture(m_gtao);
		m_d3dDevice->TransitionResource(gbufferB->m_resource.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		m_d3dDevice->TransitionResource(depth->m_resource.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		m_d3dDevice->TransitionResource(gtao->m_resource.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		gbufferB->m_state = icpResourceState::SHADER_RESOURCE;
		depth->m_state = icpResourceState::SHADER_RESOURCE;
		gtao->m_state = icpResourceState::SHADER_RESOURCE;
	}
	CompositePass(cmd, m_currentFrame);
	ForwardTranslucentPass(cmd, m_currentFrame);
	RenderImGui(cmd);
	m_d3dDevice->TransitionResource(m_d3dDevice->GetCurrentBackBuffer(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);

	m_pDevice->EndFrame();
}

void icpD3D12DeferredRenderer::UpdateSceneCB(uint32_t frameIndex)
{
	const float aspectRatio = static_cast<float>(m_pDevice->GetBackBufferWidth()) / static_cast<float>(m_pDevice->GetBackBufferHeight());
	g_system_container.m_cameraSystem->UpdateCameraCB(m_frameCB, aspectRatio);
	g_system_container.m_lightSystem->UpdateLightCB(m_frameCB);
	UpdateCSMCB(frameIndex);

	void* data = SceneUBOs[frameIndex].buffer->Map();
	memcpy(data, &m_frameCB, sizeof(m_frameCB));
	SceneUBOs[frameIndex].buffer->Unmap();
}

void icpD3D12DeferredRenderer::UpdateCSMCB(uint32_t frameIndex)
{
	const float nearPlane = g_system_container.m_configSystem->NearPlane;
	const float farPlane = g_system_container.m_configSystem->FarPlane;
	float cascadeSplits[D3D12_CSM_CASCADE_COUNT + 1]{};
	cascadeSplits[0] = nearPlane;
	cascadeSplits[D3D12_CSM_CASCADE_COUNT] = farPlane;
	for (uint32_t i = 1; i < D3D12_CSM_CASCADE_COUNT; ++i)
	{
		const float si = static_cast<float>(i) / static_cast<float>(D3D12_CSM_CASCADE_COUNT);
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

	for (uint32_t cascade = 0; cascade < D3D12_CSM_CASCADE_COUNT; ++cascade)
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
		const glm::mat4 lightProj = MakeD3D12Ortho(-radius, radius, -radius, radius, 0.f, radius * 2.f);
		m_csmData.lightViewProj[cascade] = lightProj * lightView;
	}

	void* data = m_csmUBOs[frameIndex].buffer->Map();
	memcpy(data, &m_csmData, sizeof(m_csmData));
	m_csmUBOs[frameIndex].buffer->Unmap();
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

void icpD3D12DeferredRenderer::ShadowPass(ID3D12GraphicsCommandList* cmd, uint32_t frameIndex)
{
	auto* pipeline = AsD3D12Pipeline(m_csmPipeline);
	cmd->SetPipelineState(pipeline->m_pipelineState.Get());
	cmd->SetGraphicsRootSignature(pipeline->m_rootSignature.Get());
	cmd->SetGraphicsRootConstantBufferView(1, AsD3D12Buffer(m_csmUBOs[frameIndex])->GetGPUAddress());

	D3D12_VIEWPORT viewport{ 0.f, 0.f, static_cast<float>(D3D12_CSM_RESOLUTION), static_cast<float>(D3D12_CSM_RESOLUTION), 0.f, 1.f };
	D3D12_RECT scissor{ 0, 0, static_cast<LONG>(D3D12_CSM_RESOLUTION), static_cast<LONG>(D3D12_CSM_RESOLUTION) };
	cmd->RSSetViewports(1, &viewport);
	cmd->RSSetScissorRects(1, &scissor);

	for (uint32_t cascade = 0; cascade < D3D12_CSM_CASCADE_COUNT; ++cascade)
	{
		auto* shadowMap = AsD3D12Texture(m_shadowMaps[cascade]);
		m_d3dDevice->TransitionResource(shadowMap->m_resource.Get(), m_d3dDevice->ToD3D12State(shadowMap->m_state), D3D12_RESOURCE_STATE_DEPTH_WRITE);
		shadowMap->m_state = icpResourceState::DEPTH_WRITE;

		cmd->OMSetRenderTargets(0, nullptr, FALSE, &shadowMap->m_dsv);
		cmd->ClearDepthStencilView(shadowMap->m_dsv, D3D12_CLEAR_FLAG_DEPTH, 1.f, 0, 0, nullptr);
		cmd->SetGraphicsRoot32BitConstant(2, cascade, 0);
		DrawShadowScene(cmd, frameIndex, cascade);

		m_d3dDevice->TransitionResource(shadowMap->m_resource.Get(), D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		shadowMap->m_state = icpResourceState::SHADER_RESOURCE;
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
	m_d3dDevice->TransitionResource(c->m_resource.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	if (m_enableGTAO)
	{
		m_d3dDevice->TransitionResource(b->m_resource.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		m_d3dDevice->TransitionResource(depth->m_resource.Get(), D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		if (m_gtao)
		{
			auto* gtao = AsD3D12Texture(m_gtao);
			m_d3dDevice->TransitionResource(gtao->m_resource.Get(), m_d3dDevice->ToD3D12State(gtao->m_state), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
			gtao->m_state = icpResourceState::UNORDERED_ACCESS;
		}
		b->m_state = icpResourceState::NON_PIXEL_SHADER_RESOURCE;
		depth->m_state = icpResourceState::NON_PIXEL_SHADER_RESOURCE;
	}
	else
	{
		m_d3dDevice->TransitionResource(b->m_resource.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		m_d3dDevice->TransitionResource(depth->m_resource.Get(), D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		b->m_state = icpResourceState::SHADER_RESOURCE;
		depth->m_state = icpResourceState::SHADER_RESOURCE;
	}
	a->m_state = icpResourceState::SHADER_RESOURCE;
	c->m_state = icpResourceState::SHADER_RESOURCE;
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

uint64_t icpD3D12DeferredRenderer::GTAOPass(uint32_t frameIndex)
{
	if (!m_gtaoPipeline || !m_gtao)
	{
		return 0;
	}

	auto computeList = m_pDevice->BeginAsyncCompute();
	auto* d3dComputeList = static_cast<icpD3D12CommandList*>(computeList.get())->GetNative();
	auto* pipeline = AsD3D12Pipeline(m_gtaoPipeline);
	auto* gtao = AsD3D12Texture(m_gtao);

	d3dComputeList->SetPipelineState(pipeline->m_pipelineState.Get());
	d3dComputeList->SetComputeRootSignature(pipeline->m_rootSignature.Get());
	d3dComputeList->SetComputeRootDescriptorTable(0, m_gtaoSRVGpu);
	d3dComputeList->SetComputeRootDescriptorTable(1, gtao->m_uavGpu);
	d3dComputeList->SetComputeRootConstantBufferView(2, AsD3D12Buffer(SceneUBOs[frameIndex])->GetGPUAddress());
	const uint32_t groupsX = (m_pDevice->GetBackBufferWidth() + 15u) / 16u;
	const uint32_t groupsY = (m_pDevice->GetBackBufferHeight() + 15u) / 16u;
	d3dComputeList->Dispatch(groupsX, groupsY, 1);

	return m_pDevice->EndAsyncCompute(computeList);
}

void icpD3D12DeferredRenderer::DrawShadowScene(ID3D12GraphicsCommandList* cmd, uint32_t frameIndex, uint32_t cascadeIndex)
{
	std::vector<std::shared_ptr<icpGameEntity>> roots;
	g_system_container.m_sceneSystem->getRootEntityList(roots);

	auto drawMesh = [&](auto& mesh)
	{
		if (!mesh.m_pMaterial || !mesh.m_pMaterial->m_bRenderResourcesReady ||
			mesh.m_pMaterial->m_blendMode == eMaterialBlendMode::TRANSLUCENT)
		{
			return;
		}

		cmd->SetGraphicsRootConstantBufferView(0, AsD3D12Buffer(mesh.MeshUBOs[frameIndex])->GetGPUAddress());

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
	cmd->SetGraphicsRootConstantBufferView(2, AsD3D12Buffer(m_csmUBOs[frameIndex])->GetGPUAddress());
	cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	cmd->DrawInstanced(3, 1, 0, 0);
}

void icpD3D12DeferredRenderer::ForwardTranslucentPass(ID3D12GraphicsCommandList* cmd, uint32_t frameIndex)
{
	auto* depth = AsD3D12Texture(m_depth);
	m_d3dDevice->TransitionResource(depth->m_resource.Get(), m_d3dDevice->ToD3D12State(depth->m_state), D3D12_RESOURCE_STATE_DEPTH_READ);
	depth->m_state = icpResourceState::DEPTH_READ;

	auto backBufferRTV = m_d3dDevice->GetCurrentBackBufferRTV();
	cmd->OMSetRenderTargets(1, &backBufferRTV, FALSE, &depth->m_readOnlyDsv);
	D3D12_VIEWPORT viewport{ 0.f, 0.f, static_cast<float>(m_pDevice->GetBackBufferWidth()), static_cast<float>(m_pDevice->GetBackBufferHeight()), 0.f, 1.f };
	D3D12_RECT scissor{ 0, 0, static_cast<LONG>(m_pDevice->GetBackBufferWidth()), static_cast<LONG>(m_pDevice->GetBackBufferHeight()) };
	cmd->RSSetViewports(1, &viewport);
	cmd->RSSetScissorRects(1, &scissor);

	auto* pipeline = AsD3D12Pipeline(m_translucentPipeline);
	cmd->SetPipelineState(pipeline->m_pipelineState.Get());
	cmd->SetGraphicsRootSignature(pipeline->m_rootSignature.Get());
	cmd->SetGraphicsRootConstantBufferView(3, AsD3D12Buffer(SceneUBOs[frameIndex])->GetGPUAddress());
	DrawTranslucentScene(cmd, frameIndex);

	cmd->OMSetRenderTargets(1, &backBufferRTV, FALSE, nullptr);
	m_d3dDevice->TransitionResource(depth->m_resource.Get(), D3D12_RESOURCE_STATE_DEPTH_READ, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	depth->m_state = icpResourceState::SHADER_RESOURCE;
}

void icpD3D12DeferredRenderer::DrawTranslucentScene(ID3D12GraphicsCommandList* cmd, uint32_t frameIndex)
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

void icpD3D12DeferredRenderer::RenderImGui(ID3D12GraphicsCommandList* cmd)
{
	if (!m_imguiInitialized || !m_editorUI)
	{
		return;
	}

	ImGui_ImplDX12_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
	m_editorUI->showEditorUI();
	ImGui::Render();
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), cmd);
}

INCEPTION_END_NAMESPACE
