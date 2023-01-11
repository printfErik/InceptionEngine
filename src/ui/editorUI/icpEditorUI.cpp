#include "icpEditorUI.h"
#include "../../core/icpSystemContainer.h"
#include "../../render/icpWindowSystem.h"
#include "../../scene/icpSceneSystem.h"
#include "../../resource/icpResourceSystem.h"
#include "../../scene/icpEntity.h"
#include "../../scene/icpEntityDataComponent.h"
#include "../../scene/icpXFormComponent.h"
#include "../../render/icpRenderSystem.h"

INCEPTION_BEGIN_NAMESPACE

icpEditorUI::icpEditorUI()
{
	
}

icpEditorUI::~icpEditorUI()
{
	
}

// todo
bool checkFilePath(const std::string path)
{
	return true;
}

void icpEditorUI::showEditorDockingSpaceUI()
{
	static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;
	static bool dockingSpaceOpen = true;
	// We are using the ImGuiWindowFlags_NoDocking flag to make the parent window not dockable into,
	// because it would be confusing to have two docking targets within each others.
	static ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;

	const ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(viewport->WorkPos);
	ImGui::SetNextWindowSize(viewport->WorkSize);
	ImGui::SetNextWindowViewport(viewport->ID);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
	window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

	// When using ImGuiDockNodeFlags_PassthruCentralNode, DockSpace() will render our background
	// and handle the pass-thru hole, so we ask Begin() to not render a background.
	if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
		window_flags |= ImGuiWindowFlags_NoBackground;

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	ImGui::Begin("DockSpace Demo", &dockingSpaceOpen, window_flags);
	ImGui::PopStyleVar();

	ImGui::PopStyleVar(2);

	// Submit the DockSpace
	ImGuiIO& io = ImGui::GetIO();
	if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
	{
		ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
		ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
	}

	if (ImGui::BeginMenuBar())
	{
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("Close"))
			{
				g_system_container.m_windowSystem->closeWindow();
			}
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Entity"))
		{
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Component"))
		{
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Resources"))
		{
			static char Path[64] = "";
			ImGui::InputText("Load Model File Path", Path, 64);
			if (ImGui::Button("Load Obj Files"))
			{
				if (std::strlen(Path) == 0 || !checkFilePath(std::string()))
				{
					ImGui::SameLine();
					ImGui::Text("File Path is not Legal!");
				}
				else
				{
					g_system_container.m_resourceSystem->loadObjModelResource(Path);
				}
			}

			static char ImagePath[64] = "";
			ImGui::InputText("Load Image File Path", ImagePath, 64);
			if (ImGui::Button("Load Img Files"))
			{
				if (std::strlen(ImagePath) == 0 || !checkFilePath(std::string()))
				{
					ImGui::SameLine();
					ImGui::Text("File Path is not Legal!");
				}
				else
				{
					g_system_container.m_resourceSystem->loadImageResource(ImagePath);
				}
			}
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Debug Tool"))
		{
			if (ImGui::TreeNode("Save files"))
			{
				static char buf1[64] = "";
				ImGui::InputText("Out File Path", buf1, 64);

				if (ImGui::Button("Save default scene to File"))
				{
					if (std::strlen(buf1) == 0 || !checkFilePath(std::string()))
					{
						ImGui::SameLine();
						ImGui::Text("File Path is not Legal!");
					}
					else
					{
						g_system_container.m_sceneSystem->saveScene(buf1);
					}
				}
				ImGui::TreePop();
			}

			ImGui::EndMenu();
		}

		ImGui::EndMainMenuBar();

		ImGui::ShowDebugLogWindow();
	}

	showEntityHierarchy();
}

void icpEditorUI::showEntityHierarchy()
{
	ImGuiTabBarFlags hierarchyTabBarFlags = ImGuiTabBarFlags_Reorderable;
	if (ImGui::BeginTabBar("TabBar", hierarchyTabBarFlags))
	{
		if (ImGui::BeginTabItem("Entity Hierarchy"))
		{
			std::vector<std::shared_ptr<icpGameEntity>> rootList;
			g_system_container.m_sceneSystem->getRootEntityList(rootList);

			for (auto& entity : rootList)
			{
				recursiveAddEntityToHierarchy(entity);
			}
			ImGui::EndTabItem();
		}
		ImGui::EndTabBar();
	}
}

void icpEditorUI::recursiveAddEntityToHierarchy(std::shared_ptr<icpGameEntity> entity)
{
	auto&& entityData = entity->accessComponent<icpEntityDataComponent>();
	if (ImGui::TreeNode(entityData.m_name.c_str()))
	{
		auto children = entity->accessComponent<icpXFormComponent>().m_children;
		for (auto& child: children)
		{
			recursiveAddEntityToHierarchy(child->m_entity.lock());
		}
		ImGui::TreePop();
	}
	showEntityRightClickMenu(entityData.m_name.c_str());
}

void icpEditorUI::showEntityRightClickMenu(const char* entityName)
{
	if(ImGui::BeginPopupContextItem(entityName))
	{
		if (ImGui::MenuItem("Delete"))
		{
			
		}
		if (ImGui::MenuItem("Copy"))
		{

		}
		if (ImGui::MenuItem("Cut"))
		{

		}
		ImGui::EndPopup();
	}
}

void icpEditorUI::showEditorUI()
{
	if (ImGui::BeginMainMenuBar())
	{
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("Close"))
			{
				g_system_container.m_windowSystem->closeWindow();
			}
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Entity"))
		{
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Component"))
		{
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Resources"))
		{
			static char Path[64] = "";
			ImGui::InputText("Load Model File Path", Path, 64);
			if (ImGui::Button("Load Obj Files"))
			{
				if (std::strlen(Path) == 0 || !checkFilePath(std::string()))
				{
					ImGui::SameLine();
					ImGui::Text("File Path is not Legal!");
				}
				else
				{
					auto modelResource = g_system_container.m_resourceSystem->loadObjModelResource(Path,true);
					g_system_container.m_sceneSystem->createMeshEnityFromResource(modelResource);
				}
			}

			static char ImagePath[64] = "";
			ImGui::InputText("Load Image File Path", ImagePath, 64);
			if (ImGui::Button("Load Img Files"))
			{
				if (std::strlen(ImagePath) == 0 || !checkFilePath(std::string()))
				{
					ImGui::SameLine();
					ImGui::Text("File Path is not Legal!");
				}
				else
				{
					g_system_container.m_resourceSystem->loadImageResource(ImagePath);
				}
			}
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Debug Tool"))
		{
			if (ImGui::TreeNode("Save files"))
			{
				static char buf1[64] = "";
				ImGui::InputText("Out File Path", buf1, 64);

				if (ImGui::Button("Save default scene to File"))
				{
					if (std::strlen(buf1) == 0 || !checkFilePath(std::string()))
					{
						ImGui::SameLine();
						ImGui::Text("File Path is not Legal!");
					}
					else
					{
						g_system_container.m_sceneSystem->saveScene(buf1);
					}
				}
				ImGui::TreePop();
			}

			ImGui::EndMenu();
		}

		ImGui::EndMainMenuBar();
	}

	showEntityHierarchy();
}


void icpEditorUI::drawCube()
{
	g_system_container.m_renderSystem->drawCube();
}


INCEPTION_END_NAMESPACE